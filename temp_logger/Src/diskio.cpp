

#include "diskio.h"
#include "spi.hpp"
#include "systick.hpp"

extern Spi sdSpi;

// SD card SPI protocol constants

static constexpr u8  SD_DUMMY      = 0xFF;   // idle byte (MOSI high)
static constexpr u8  CMD0          = 0x40;   // GO_IDLE_STATE
static constexpr u8  CMD1          = 0x41;   // SEND_OP_COND (MMC)
static constexpr u8  CMD8          = 0x48;   // SEND_IF_COND (SDv2)
static constexpr u8  CMD17         = 0x51;   // READ_SINGLE_BLOCK
static constexpr u8  CMD24         = 0x58;   // WRITE_BLOCK
static constexpr u8  CMD55         = 0x77;   // APP_CMD prefix
static constexpr u8  CMD58         = 0x7A;   // READ_OCR
static constexpr u8  ACMD41        = 0x69;   // SD_SEND_OP_COND
static constexpr u8  DATA_TOKEN    = 0xFE;   // single block data token
static constexpr u32 BLOCK_SIZE    = 512;
static constexpr u32 INIT_TIMEOUT  = 2000;   // ms

// Card type flags (kept in module-level state)
static volatile DSTATUS diskStatus = STA_NOINIT;
static u8 cardType = 0;   // 0=unknown, 1=SDv1, 2=SDv2, 4=SDHC


// Send one byte over SPI and return the received byte
static u8 spiTransfer(u8 tx)
{
    u8 rx;
    sdSpi.transmit(&tx, 1);
    sdSpi.receive(&rx, 1);
    return rx;
}

// Send 0xFF bytes until we get a non-0xFF response or timeout (ms)
static u8 waitReady(u32 timeoutMs)
{
    u8  resp;
    u32 start = Tick::getTick();
    do {
        resp = spiTransfer(SD_DUMMY);
    } while (resp != 0xFF && Tick::elapsed(start) < timeoutMs);
    return resp;
}

// Send a 6-byte SD command (cmd | 0x40, 4-byte arg, CRC) and return R1
static u8 sendCommand(u8 cmd, u32 arg)
{
    // CMD55 prefix needed for ACMD41
    if (cmd == ACMD41) {
        u8 r = sendCommand(CMD55, 0);
        if (r > 1) return r;
    }

    // Assert CS and wait for card to be ready
    sdSpi.csEnable();
    waitReady(500);

    // Command frame: start bit=0, tx bit=1, cmd[5:0], arg[31:0], CRC[6:0], stop=1
    u8 frame[6];
    frame[0] = cmd;
    frame[1] = static_cast<u8>(arg >> 24);
    frame[2] = static_cast<u8>(arg >> 16);
    frame[3] = static_cast<u8>(arg >>  8);
    frame[4] = static_cast<u8>(arg);
    // CRC required only for CMD0 and CMD8 in SPI mode; others can use 0x01
    frame[5] = (cmd == CMD0) ? 0x95 :
               (cmd == CMD8) ? 0x87 : 0x01;
    sdSpi.transmit(frame, 6);

    // Read R1 response (first byte with MSB=0, up to 8 attempts)
    u8 resp = 0xFF;
    for (int i = 0; i < 8; i++) {
        resp = spiTransfer(SD_DUMMY);
        if (!(resp & 0x80)) break;
    }
    return resp;
}

// disk_initialize: bring the SD card into SPI mode and determine card type
DSTATUS disk_initialize(BYTE drv)
{
    if (drv != 0) return STA_NOINIT;   // only one drive

    // Clock card at low speed during init (~400 kHz, Div256 on 16 MHz HSI)
    sdSpi.setBaudRate(BaudRate::Div256);
    sdSpi.csDisable();

    // Send ≥74 dummy clocks with CS deasserted (SD card power-up requirement)
    u8 dummy = SD_DUMMY;
    for (int i = 0; i < 10; i++) sdSpi.transmit(&dummy, 1);

    // CMD0 -> idle state (R1 = 0x01)
    u8 resp = sendCommand(CMD0, 0);
    if (resp != 0x01) {
        sdSpi.csDisable();
        return STA_NOINIT;
    }

    // CMD8 -> check for SDv2 (arg = 0x1AA, check pattern)
    resp = sendCommand(CMD8, 0x000001AA);
    if (resp == 0x01) {
        // SDv2: read 4-byte R7, check voltage accepted
        u8 r7[4];
        for (int i = 0; i < 4; i++) r7[i] = spiTransfer(SD_DUMMY);
        if (r7[3] == 0xAA) {
            // ACMD41 with HCS bit -> initialise
            u32 start = Tick::getTick();
            do {
                resp = sendCommand(ACMD41, 0x40000000);
            } while (resp != 0 && Tick::elapsed(start) < INIT_TIMEOUT);

            if (resp == 0) {
                // CMD58: read OCR, check CCS bit for SDHC
                resp = sendCommand(CMD58, 0);
                u8 ocr[4];
                for (int i = 0; i < 4; i++) ocr[i] = spiTransfer(SD_DUMMY);
                cardType = (ocr[0] & 0x40) ? 4 : 2;   // SDHC or SDv2
            }
        }
    } else {
        // SDv1 or MMC: initialize with ACMD41 / CMD1
        u32 start = Tick::getTick();
        do {
            resp = sendCommand(ACMD41, 0);
        } while (resp != 0 && Tick::elapsed(start) < INIT_TIMEOUT);

        if (resp != 0) {
            // Try CMD1 for MMC
            start = Tick::getTick();
            do {
                resp = sendCommand(CMD1, 0);
            } while (resp != 0 && Tick::elapsed(start) < INIT_TIMEOUT);
        }
        cardType = (resp == 0) ? 1 : 0;
    }

    sdSpi.csDisable();
    spiTransfer(SD_DUMMY);   // 8 extra clocks to release card DO

    if (cardType == 0) return STA_NOINIT;

    // Initialized successfully — switch to full speed (Div4 ≈ 4 MHz)
    sdSpi.setBaudRate(BaudRate::Div4);
    diskStatus = 0;
    return diskStatus;
}

// disk_status: return current status
DSTATUS disk_status(BYTE drv)
{
    return (drv == 0) ? diskStatus : STA_NOINIT;
}

// disk_read: read one or more 512-byte sectors
DRESULT disk_read(BYTE drv, BYTE* buff, LBA_t sector, UINT count)
{
    if (drv != 0 || diskStatus & STA_NOINIT) return RES_NOTRDY;

    // SDSC uses byte addressing; SDHC/SDXC uses sector addressing
    if (cardType != 4) sector *= BLOCK_SIZE;

    while (count--) {
        u8 resp = sendCommand(CMD17, sector);
        if (resp != 0x00) { sdSpi.csDisable(); return RES_ERROR; }

        // Wait for data token (0xFE)
        u32 start = Tick::getTick();
        u8  token = 0xFF;
        do {
            token = spiTransfer(SD_DUMMY);
        } while (token == 0xFF && Tick::elapsed(start) < 500);

        if (token != DATA_TOKEN) { sdSpi.csDisable(); return RES_ERROR; }

        // Read 512 bytes + 2 CRC bytes (CRC ignored in SPI mode)
        sdSpi.receive(buff, BLOCK_SIZE);
        spiTransfer(SD_DUMMY);   // CRC high
        spiTransfer(SD_DUMMY);   // CRC low

        buff   += BLOCK_SIZE;
        sector += (cardType == 4) ? 1 : BLOCK_SIZE;
        sdSpi.csDisable();
        spiTransfer(SD_DUMMY);
    }
    return RES_OK;
}

// disk_write: write one or more 512-byte sectors
DRESULT disk_write(BYTE drv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (drv != 0 || diskStatus & STA_NOINIT) return RES_NOTRDY;

    if (cardType != 4) sector *= BLOCK_SIZE;

    while (count--) {
        u8 resp = sendCommand(CMD24, sector);
        if (resp != 0x00) { sdSpi.csDisable(); return RES_ERROR; }

        // Send data token, then 512 bytes, then 2 dummy CRC bytes
        u8 token = DATA_TOKEN;
        sdSpi.transmit(&token, 1);
        sdSpi.transmit(const_cast<u8*>(buff), BLOCK_SIZE);
        spiTransfer(SD_DUMMY);   // CRC high (don't care)
        spiTransfer(SD_DUMMY);   // CRC low

        // Data response token — lower nibble 0x5 = accepted
        resp = spiTransfer(SD_DUMMY);
        if ((resp & 0x1F) != 0x05) { sdSpi.csDisable(); return RES_ERROR; }

        // Wait for card to finish programming (busy = MISO low)
        u32 start = Tick::getTick();
        while (spiTransfer(SD_DUMMY) == 0x00) {
            if (Tick::elapsed(start) > 500) { sdSpi.csDisable(); return RES_ERROR; }
        }

        buff   += BLOCK_SIZE;
        sector += (cardType == 4) ? 1 : BLOCK_SIZE;
        sdSpi.csDisable();
        spiTransfer(SD_DUMMY);
    }
    return RES_OK;
}

// disk_ioctl: control functions required by FatFs
DRESULT disk_ioctl(BYTE drv, BYTE cmd, void* buff)
{
    if (drv != 0 || diskStatus & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            // Wait until card is not busy
            sdSpi.csEnable();
            waitReady(500);
            sdSpi.csDisable();
            return RES_OK;

        case GET_SECTOR_SIZE:
            *reinterpret_cast<WORD*>(buff) = BLOCK_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *reinterpret_cast<DWORD*>(buff) = 1;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

// get_fattime: return current time stamp packed into a DWORD
// Format: bits[31:25]=year-1980, [24:21]=month, [20:16]=day,
//         [15:11]=hour, [10:5]=minute, [4:0]=second/2
// No RTC, so return a fixed time stamp (2026-01-01 00:00:00)
DWORD get_fattime(void)
{
    return ((DWORD)(2026 - 1980) << 25) |   // year
           ((DWORD)1             << 21) |   // month
           ((DWORD)1             << 16);    // day
}

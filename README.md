# STM32F446RE Bare Metal Temperature Logger

A bare metal embedded systems project written in C++ for the STM32F446RE Nucleo board. Samples temperature and humidity from a DHT22 sensor every 5 minutes and logs readings to a CSV file on an SD card over SPI. No STM32 HAL or vendor middleware — all peripheral access is implemented at the register level using CMSIS definitions only.

## Features

- Custom HAL classes for GPIO, RCC, SPI, EXTI, SysTick/DWT, and USART2
- DHT22 (AM2302) single-wire driver using DWT cycle counter for microsecond timing
- FatFs SD card logging via a custom SPI diskio layer
- Two-state FSM (IDLE / SAMPLE) toggled by the onboard blue button via EXTI interrupt
- UART debug output over ST-Link virtual COM port at 115200 baud
- Host-side emulation build with 7 unit tests (no hardware required)
- QEMU support for verifying startup and FSM behaviour on emulated Cortex-M4

## Hardware

| Component | Interface | Pin(s) |
|-----------|-----------|--------|
| STM32F446RE Nucleo | — | — |
| DHT22 / AM2302 | 1-wire bit-bang | PA1 (4.7 kΩ pull-up to 3.3 V) |
| SD card module | SPI1 | PA5 SCK / PA6 MISO / PA7 MOSI / PA9 CS |
| LED 1 (IDLE) | GPIO output | PA8 (220 Ω series resistor) |
| LED 2 (SAMPLE) | GPIO output | PB5 (220 Ω series resistor) |
| User button | EXTI falling edge | PC13 (onboard) |
| Debug UART | USART2 | PA2 TX / PA3 RX (ST-Link virtual COM) |

## Project Structure

```
temp_logger/
├── Inc/
│   ├── gpio.hpp          # GPIO HAL class
│   ├── rcc.hpp           # RCC clock enable class
│   ├── spi.hpp           # SPI1 HAL class
│   ├── exti.hpp          # EXTI interrupt controller class
│   ├── systick.hpp       # SysTick + DWT tick and delay
│   ├── uart.hpp          # USART2 polling debug output
│   ├── dht22.hpp         # DHT22 single-wire sensor driver
│   ├── logger.hpp        # CSV logger wrapping FatFs
│   ├── fsm.hpp           # Two-state finite state machine
│   └── types.hpp         # Project-wide type aliases
├── Src/
│   ├── main.cpp          # System init and main loop
│   ├── diskio.cpp        # FatFs diskio glue layer for SPI SD card
│   ├── rcc.cpp           # Rcc static member definition
│   ├── systick.cpp       # Tick static member definition
│   ├── ff.c              # FatFs core (ChaN, unmodified)
│   └── ffunicode.c       # FatFs unicode (ChaN, unmodified)
├── emulate/
│   ├── hal_mock.hpp          # Fake CMSIS peripheral structs for host build
│   ├── systick_emulate.hpp   # std::chrono-based Tick for host build
│   ├── main_emulate.cpp      # Host test harness — 7 unit tests
│   └── Makefile              # Host emulation build
└── Makefile              # QEMU targets
```

## Building

### STM32 (CubeIDE)

1. Open the project in STM32CubeIDE
2. **Project → Build All** (`Ctrl+B`)
3. Output: `Debug/temp_logger.elf`

### Host Emulation (Linux / Mac)

Requires `g++` with C++17 support.

```bash
cd emulate/
make run
```

This compiles and runs 7 unit tests covering FSM transitions, LED output logic, sensor conversion math, and CSV formatting. No hardware required.

## Running

### On Hardware

1. Flash via CubeIDE (`Run → Debug`)
2. Open a serial monitor at **115200 baud** on the ST-Link virtual COM port (`/dev/ttyACM0` on Linux)
3. Press the blue USER button (B1) to toggle between IDLE and SAMPLE modes
4. In SAMPLE mode, the DHT22 is read every 5 minutes and a row is appended to `templog.csv` on the SD card

### QEMU Emulation

Requires `qemu-system-arm` (`sudo apt install qemu-system-arm`).

```bash
make check   # verify QEMU is installed and ELF exists
make qemu    # boot the ELF, USART2 output printed to terminal
```

Press `Ctrl+A` then `X` to quit QEMU.

Expected output:
```
=== STM32 Temp Logger ===
Initialising...
Tick: OK
SPI: OK
DHT22: OK
Logger: FAILED (no SD card?)
EXTI: OK
[FSM] -> IDLE   (LED1=ON LED2=OFF)
FSM: OK -- entering IDLE
Press button to toggle SAMPLE mode.
```

`Logger: FAILED` is expected in QEMU — there is no SD card peripheral to emulate. Everything above it confirms correct startup, clock init, and FSM behaviour on emulated Cortex-M4 silicon.

## CSV Output Format

```
elapsed_ms,temperature_c,humidity_pct
300000,23.5,58.2
600000,23.6,58.0
```

## What Was Tested

| Method | What it covers |
|--------|---------------|
| QEMU | Startup, vector table, SysTick, peripheral clock init, FSM IDLE entry, UART TX |
| Host emulation | FSM transitions, LED register writes, 5-minute timer, DHT22 conversion math, CSV format |
| Hardware | Not completed (resistors unavailable during testing window) |

## Dependencies

- STM32CubeIDE 2.1.0
- arm-none-eabi-gcc 14.3.1 (bundled with CubeIDE)
- FatFs R0.15 (ChaN) — included in `Src/`
- CMSIS 5 headers for STM32F446 — included in `Inc/STM32F4/`
- QEMU 9.2.4 (`qemu-system-arm`) for emulation
- g++ with C++17 for host emulation build
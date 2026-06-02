

#ifndef SPI_HPP_
#define SPI_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"
#include "gpio.hpp"
#include "rcc.hpp"

enum class BaudRate : u8 {
	Div2 = 0b000,
	Div4 = 0b001,
	Div8 = 0b010,
	Div16 = 0b011,
	Div32 = 0b100,
	Div64 = 0b101,
	Div128 = 0b110,
	Div256 = 0b111
};

enum class ClockPolarity : u8 {
	Low = 0,
	High = 1
};

enum class ClockPhase : u8 {
	FirstEdge = 0,
	SecondEdge = 1,
};

enum class RxOnly : u8 {
	FullDuplex = 0, // Transmit and receive
	OutputDisabled = 1
};

enum class BitOrder : u8 {
	MSBFirst = 0,
	LSBFirst = 1
};

enum class MasterSelection : u8 {
	Slave = 0,
	Master = 1
};

enum class DataFrameFormat : u8 {
	Bit8 = 0,
	Bit16 = 1
};

enum class SoftwareSlaveManagement : u8 {

	// Slave select is controlled by negative slave select pin
	Disabled = 0,

	// Slave select is controlled software-controlled internal value
	Enabled = 1
};

/*
 * This bit has an effect only when the SSM bit is set
 * The value of this bit is forced onto the NSS pin and the
 * IO value of the NSS pin is ignored
 */
enum class InternalSlaveSelect : u8 {
	NSSPin = 0,
	Internal = 1
};

struct SpiPins{
	Pin sck;
	Pin miso;
	Pin mosi;
	Pin* nss = nullptr;
};

class Spi {
public:
	explicit Spi(SPI_TypeDef* spi, SpiPins pins)
		: spi(spi),
		  pins(pins)
	{
	}

	void setBaudRate(BaudRate br)
	{
		spi->CR1 = (spi->CR1 & ~CR1_BR_Msk) |
				   (static_cast<u32>(br) << CR1_BR_Pos);
	}

	void setClockPolarity(ClockPolarity cpol)
	{

		spi->CR1 = (spi->CR1 & ~CR1_CPOL_Msk) |
				   (cpol == ClockPolarity::High ? CR1_CPOL_Msk : 0);
	}

	void setClockPhase (ClockPhase cpha)
	{
		spi->CR1 = (spi->CR1 & ~CR1_CPHA_Msk) |
				   (cpha == ClockPhase::SecondEdge ? CR1_CPHA_Msk : 0);
	}

	void setRxOnly (RxOnly rxonly)
	{
		spi->CR1 = (spi->CR1 & ~CR1_RXONLY_Msk) |
				   (rxonly == RxOnly::OutputDisabled ? CR1_RXONLY_Msk : 0);

	}

	void setBitOrder (BitOrder bitorder)
	{
		spi->CR1 = (spi->CR1 & ~CR1_LSBFIRST_Msk) |
				   (bitorder == BitOrder::LSBFirst ? CR1_LSBFIRST_Msk : 0);
	}

	void setMasterSelection (MasterSelection masterSelection)
	{
		spi->CR1 = (spi->CR1 & ~CR1_MSTR_Msk) |
				   (masterSelection == MasterSelection::Master ? CR1_MSTR_Msk : 0);
	}

	void setDataFrameFormat (DataFrameFormat dff)
	{
		spi->CR1 = (spi->CR1 & ~CR1_DFF_Msk) |
				   (dff == DataFrameFormat::Bit16 ? CR1_DFF_Msk : 0);
	}

	void setInternalSlaveSelect (InternalSlaveSelect ssi)
	{
		spi->CR1 = (spi->CR1 & ~CR1_SSI_Msk) |
				   (ssi == InternalSlaveSelect::Internal ? CR1_SSI_Msk : 0);
	}

	void setSoftwareSlaveManagement (SoftwareSlaveManagement ssm)
	{
			spi->CR1 = (spi->CR1 & ~CR1_SSM_Msk) |
					   (ssm == SoftwareSlaveManagement::Enabled ? CR1_SSM_Msk : 0);
	}

	void enableSpi ()
	{
		if (spi == SPI1)
		{
			spi->CR1 |= CR1_SPE_Msk;

		}
	}

	void disableSpi ()
	{
		if (spi == SPI1)
		{
			spi->CR1 &= ~CR1_SPE_Msk;

		}
	}

	void initGpio()
	{

		if (spi == SPI1){
			rcc.enableAhb1(RCC_AHB1ENR::RCC_GPIOA);

			pins.sck.gpio->setMode(pins.sck.pin, Mode::Alt);
			pins.miso.gpio->setMode(pins.miso.pin, Mode::Alt);
			pins.mosi.gpio->setMode(pins.mosi.pin, Mode::Alt);
			pins.nss->gpio->setMode(pins.nss->pin, Mode::Output);

			// Set pins to alternate function mode AF5 (SPI)
			pins.sck.gpio->setAlternateFunction(pins.sck.pin, AF::AF5);
			pins.miso.gpio->setAlternateFunction(pins.miso.pin, AF::AF5);
			pins.mosi.gpio->setAlternateFunction(pins.mosi.pin, AF::AF5);
		}
	}

	void config()
	{
		rcc->enableApb2(RCC_APB2ENR::RCC_SPI1);

		setBaudRate(BaudRate::Div4);
		setClockPolarity(ClockPolarity::High);
		setClockPhase(ClockPhase::SecondEdge);
		setRxOnly(RxOnly::FullDuplex);
		setBitOrder(BitOrder::MSBFirst);
		setMasterSelection(MasterSelection::Master);
		setDataFrameFormat(DataFrameFormat::Bit8);
		setSoftwareSlaveManagement(SoftwareSlaveManagement::Enabled);
		setInternalSlaveSelect(InternalSlaveSelect::Internal);
		enableSpi();
	}

	//Chip select enable
	void csEnable(){
		if (spi == SPI1){
			pins.nss->gpio->ODR &= ~(1U << 9);
		}
	}

	//Pull high to disable
	void csDisable(){
		if (spi == SPI1){
			pins.nss->gpio->ODR |= (1U >> 9);
		}
	}

	void transmit(u8 *data, u32 size)
	{
		u32 i=0;
		u8 temp;

		while(i<size)
		{
			//Wait until transmit buffer empty bit is set
			while (!(spi->SR & (SR_TXE))){}

			//Write the data to the data register
			spi->DR = data[i];
			i++;

			// Wait until a byte is received
			        while(!(spi->SR & SR_RXNE)){}

			// Read and discard received byte
			temp = spi->DR;
		}

		//Wait until TXE is set
		//Wait for BUSY flag to reset
		while((spi->SR & (SR_BSY))){}

		//Clear OVR flag
		temp = spi->DR;
		temp = spi->SR;

	}

	void receive(u8 *data, u32 size)
	{
		while(size)
		{
			/*Send dummy data
			Master must transmit something before SPI can begin to start receiving
			data from slave*/
			spi->DR = 0;

			//Wait for receive to complete
			while(!(spi->SR & (SR_RXNE))){}

			//Read data from data register
			*data++ = (spi->DR);
			size--;
		}
	}

private:
SPI_TypeDef* const spi;
SpiPins pins;

static constexpr u32 CR1_BR_Pos = 3;
static constexpr u32 CR1_BR_Msk = (0x7U << CR1_BR_Pos);

static constexpr u32 CR1_CPOL_Pos = 1;
static constexpr u32 CR1_CPOL_Msk = (1U << CR1_CPOL_Pos);

static constexpr u32 CR1_CPHA_Pos = 0;
static constexpr u32 CR1_CPHA_Msk = (1U << CR1_CPHA_Pos);

static constexpr u32 CR1_RXONLY_Pos = 10;
static constexpr u32 CR1_RXONLY_Msk = (1U << CR1_RXONLY_Pos);

static constexpr u32 CR1_LSBFIRST_Pos = 7;
static constexpr u32 CR1_LSBFIRST_Msk = (1U << CR1_LSBFIRST_Pos);

static constexpr u32 CR1_MSTR_Pos = 2;
static constexpr u32 CR1_MSTR_Msk = (1U << CR1_MSTR_Pos);

static constexpr u32 CR1_SPE_Pos = SPE;
static constexpr u32 CR1_SPE_Msk = (1U << CR1_SPE_Pos);

static constexpr u32 CR1_DFF_Pos = 11;
static constexpr u32 CR1_DFF_Msk = (1U << CR1_DFF_Pos);

static constexpr u32 CR1_SSM_Pos = 9;
static constexpr u32 CR1_SSM_Msk = (1U << CR1_SSM_Pos);

static constexpr u32 CR1_SSI_Pos = 8;
static constexpr u32 CR1_SSI_Msk = (1U << CR1_SSI_Pos);

static constexpr u32 SR_RXNE = (1U << 0);
static constexpr u32 SR_TXE = (1U << 1);
static constexpr u32 SR_BSY = (1U << 7);

};



#endif /* SPI_HPP_ */

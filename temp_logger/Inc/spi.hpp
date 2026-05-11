

#ifndef SPI_HPP_
#define SPI_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"

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

class Spi {
public:
	explicit Spi(SPI_TypeDef* spi)
		: spi(spi) {}

	static constexpr u32 CR1_BR_Pos = 3;
	static constexpr u32 CR1_BR_Msk = (1U << CR1_BR_Msk);

	static constexpr u32 CR1_CPOL_Pos = 1;
	static constexpr u32 CR1_CPOL_Msk = (1U << CR1_CPOL_Pos);

	static constexpr u32 CR1_CPHA_Pos = 0;
	static constexpr u32 CR1_CPHA_Msk = (1U << CR1_CPHA_Pos);

	static constexpr u32 CR1_RXONLY_Pos = 10;
	static constexpr u32 CR1_RXONLY_Msk = (1U << CR1_RXONLY_Msk);

	static constexpr u32 CR1_LSBFIRST_Pos = 7;
	static constexpr u32 CR1_LSBFIRST_Msk = (1U << CR1_LSBFIRST_Pos);

	static constexpr u32 CR1_MSTR_Pos = 2;
	static constexpr u32 CR1_MSTR_Msk = (1U << CR1_MSTR_Pos);

	static constexpr u32 CR1_SPE_Pos = SPE;
	static constexpr u32 CR1_SPE_Msk = (1U << CR1_SPE_Pos);

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

	void setRXOnly (RxOnly rxonly)
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



private:
SPI_TypeDef* const spi;
};



#endif /* SPI_HPP_ */

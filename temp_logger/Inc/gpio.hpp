

#ifndef STM32F4_GPIO_HPP_
#define STM32F4_GPIO_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"
#include "rcc.hpp"

class Gpio;

enum class Mode : u8 {
	Input = 0b00,
	Output = 0b01,
	Alt = 0b10,
	Analog = 0b11
};

enum class Speed : u8 {
	Low = 0b00,
	Medium = 0b01,
	High = 0b10,
	Fast = 0b11
};

enum class Pull : u8 {
	None = 0b00,
	Up = 0b01,
	Down = 0b10
};

enum class AF : u8 {
    AF0 = 0x0 , AF1 = 0x1, AF2 = 0x2, AF3 = 0x3,
    AF4 = 0x4 , AF5 = 0x5 , AF6 = 0x6, AF7 = 0x7,
    AF8 = 0x8, AF9 = 0x9, AF10 = 0xA, AF11 = 0xB,
    AF12 = 0xC, AF13 = 0xD, AF14 = 0xE, AF15 = 0xF
};

struct Pin {
	Gpio* gpio;
	u8 pin;
	RCC_AHB1ENR rccMask;
};

class Gpio {
public:
	explicit Gpio(GPIO_TypeDef* port)
		: port(port) {}

	void setMode(u8 pin, Mode mode){
		u32 shift = pin * 2;
		u32 mask = 0x3 << shift;

		port->MODER = (port->MODER & ~mask) |
						(static_cast<u32>(mode) << shift);
	}

	void setPull(u8 pin, Pull pull){
		u32 shift = pin * 2;
		u32 mask = 0x3 << shift;

		port->PUPDR = (port->PUPDR & ~mask) |
						(static_cast<u32>(pull) << shift);
	}

	void setSpeed(u8 pin, Speed speed)
	{
		u32 shift = pin * 2;
		u32 mask = 0x3 << shift;

		port->OSPEEDR = (port->OSPEEDR & ~mask) |
						(static_cast<u32>(speed) << shift);
	}

	void setAlternateFunction(u8 pin, AF af)
	{
		u32 shift = (pin % 8) * 4;
		u32 mask = 0xF << shift;

		if(pin < 8){
			port->AFR[0] = (port->AFR[0] & ~mask) |
							(static_cast<u32>(af) << shift);
		}
		else{
			port->AFR[1] = (port->AFR[1] & ~mask) |
							(static_cast<u32>(af) << shift);
		}
	}

	GPIO_TypeDef* getPort(){
		return port;
	}

private:
	GPIO_TypeDef* const port;
};

#endif /* STM32F4_GPIO_HPP_ */

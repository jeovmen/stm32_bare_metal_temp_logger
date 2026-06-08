/*
 * exti.hpp
 *
 *  Created on: Jun 2, 2026
 *      Author: jeovmen
 */

#ifndef EXTI_HPP_
#define EXTI_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"
#include "gpio.hpp"
#include "rcc.hpp"

enum class SYSCFG_EXTICR : u32 {
	PA = 0b0000, PB = 0b0001, PC = 0b0010, PD = 0b0011, PE = 0b0100,
	PF = 0b0101, PG = 0b0110
};

class Exti {
public:
	explicit Exti (Pin pin)
		: pin(pin) {}

	void init(){
		__disable_irq();
		rcc.enableAhb1(pin.rccMask);
		rcc.enableApb2(RCC_APB2ENR::RCC_SYSCFG);
		pin.gpio->setMode(pin.pin, Mode::Input);

	}

	void configureExtiSource()
	{

	}

private:
	Pin const pin;
};


#endif /* EXTI_HPP_ */



#ifndef RCC_HPP_
#define RCC_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"

enum class RCC_AHB1ENR : u32 {
	RCC_GPIOA = (1U << 0), RCC_GPIOB = (1U << 1), RCC_GPIOC = (1U << 2),
	RCC_GPIOD = (1U << 3), RCC_GPIOE = (1U << 4), RCC_GPIOF = (1U << 5),
	RCC_GPIOG = (1U << 6), RCC_GPIOH = (1U << 7), RCC_DMA1 = (1U << 21),
	RCC_DMA2 = (1U << 22)
};

enum class RCC_APB2ENR : u32 {
	RCC_TIM1 = (1U << 0), RCC_TIM8 = (1U << 1), RCC_USART1 = (1U << 4),
	RCC_USART2 = (1U << 5), RCC_ADC1 = (1U << 8), RCC_ADC2 = (1U << 9),
	RCC_ADC3 = (1U << 10), RCC_SDIO = (1U << 11), RCC_SPI1 = (1U << 12),
	RCC_SPI4 = (1U << 13), RCC_SYSCFG = (1U << 14)
};

class Rcc {
public:

	void resetAhb1(){
		RCC->AHB1ENR  = 0x0;
	}

	void enableAhb1(RCC_AHB1ENR periph){
		RCC->AHB1ENR |= static_cast<u32>(periph);
	}

	void resetApb2(){
		RCC->APB2ENR = 0x0;
	}
	void enableApb2(RCC_APB2ENR periph){
			RCC->APB2ENR |= static_cast<u32>(periph);
	}
};

extern Rcc rcc;

#endif /* RCC_HPP_ */

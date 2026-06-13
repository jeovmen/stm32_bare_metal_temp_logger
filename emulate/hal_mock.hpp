/*
 * hal_mock.hpp
 *
 * Provides fake CMSIS types, register structs, and HAL stubs so that
 * gpio.hpp, spi.hpp, exti.hpp, systick.hpp, dht22.hpp, fsm.hpp, and
 * logger.hpp all compile and run on the host (Linux/Mac/Windows) without
 * any real STM32 hardware.
 *
 * Include this INSTEAD of stm32f446xx.h when building with -DEMULATE.
 * The emulate/main_emulate.cpp file handles this automatically.
 *
 *  Created on: Jun 3, 2026
 *      Author: jeovmen
 */

#ifndef HAL_MOCK_HPP_
#define HAL_MOCK_HPP_

#include <cstdint>
#include <cstdio>
#include <chrono>


using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i16 = std::int16_t;
using i32 = std::int32_t;

#define __IO volatile
#define __disable_irq()   do {} while(0)
#define __enable_irq()    do {} while(0)


struct GPIO_TypeDef {
    __IO u32 MODER;
    __IO u32 OTYPER;
    __IO u32 OSPEEDR;
    __IO u32 PUPDR;
    __IO u32 IDR;
    __IO u32 ODR;
    __IO u32 BSRR;
    __IO u32 AFR[2];
};


struct SPI_TypeDef {
    __IO u32 CR1;
    __IO u32 CR2;
    __IO u32 SR;
    __IO u32 DR;
};


struct RCC_TypeDef {
    __IO u32 AHB1ENR;
    __IO u32 APB1ENR;
    __IO u32 APB2ENR;
};


struct EXTI_TypeDef {
    __IO u32 IMR;
    __IO u32 EMR;
    __IO u32 RTSR;
    __IO u32 FTSR;
    __IO u32 SWIER;
    __IO u32 PR;
};


struct SYSCFG_TypeDef {
    __IO u32 MEMRMP;
    __IO u32 PMC;
    __IO u32 EXTICR[4];
};


struct DWT_Type {
    __IO u32 CTRL;
    __IO u32 CYCCNT;
};

struct CoreDebug_Type {
    __IO u32 DEMCR;
};

#define CoreDebug_DEMCR_TRCENA_Msk  (1U << 24)

// Static fake peripheral instances
// All HAL classes hold pointers to these — writes go into the struct fields
// which can be inspected in the emulation main for testing.

inline GPIO_TypeDef   _fakeGPIOA{}, _fakeGPIOB{}, _fakeGPIOC{};
inline SPI_TypeDef    _fakeSPI1{};
inline RCC_TypeDef    _fakeRCC{};
inline EXTI_TypeDef   _fakeEXTI{};
inline SYSCFG_TypeDef _fakeSYSCFG{};
inline DWT_Type       _fakeDWT{};
inline CoreDebug_Type _fakeCoreDebug{};

#define GPIOA       (&_fakeGPIOA)
#define GPIOB       (&_fakeGPIOB)
#define GPIOC       (&_fakeGPIOC)
#define SPI1        (&_fakeSPI1)
#define RCC         (&_fakeRCC)
#define EXTI        (&_fakeEXTI)
#define SYSCFG      (&_fakeSYSCFG)
#define DWT         (&_fakeDWT)
#define CoreDebug   (&_fakeCoreDebug)

// SystemCoreClock — pretend 16 MHz for delay math
// (delayUs still works; just uses host CPU cycles which don't matter in emulation)

inline u32 SystemCoreClock = 16000000U;

// SysTick_Config stub — no-op in emulation (Tick class uses std::chrono instead)

inline int SysTick_Config(u32) { return 0; }


// NVIC stubs

using IRQn_Type = int;
inline void NVIC_SetPriority(IRQn_Type, u32) {}
inline void NVIC_EnableIRQ(IRQn_Type) {}

// IRQn values (values don't matter for emulation)
#define EXTI0_IRQn       0
#define EXTI1_IRQn       1
#define EXTI2_IRQn       2
#define EXTI3_IRQn       3
#define EXTI4_IRQn       4
#define EXTI9_5_IRQn     5
#define EXTI15_10_IRQn   6


// RCC enum values (must match rcc.hpp — duplicated here for the host build)

enum class RCC_AHB1ENR : u32 {
    RCC_GPIOA = (1U << 0), RCC_GPIOB = (1U << 1), RCC_GPIOC = (1U << 2),
    RCC_GPIOD = (1U << 3), RCC_GPIOE = (1U << 4), RCC_GPIOF = (1U << 5),
    RCC_GPIOG = (1U << 6), RCC_GPIOH = (1U << 7)
};

enum class RCC_APB2ENR : u32 {
    RCC_TIM1 = (1U << 0), RCC_SPI1 = (1U << 12), RCC_SYSCFG = (1U << 14)
};

#endif /* HAL_MOCK_HPP_ */

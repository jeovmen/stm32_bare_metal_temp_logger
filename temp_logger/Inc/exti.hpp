

#ifndef EXTI_HPP_
#define EXTI_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"
#include "gpio.hpp"
#include "rcc.hpp"

enum class ExtiTrigger : u8 {
    Rising  = 0,
    Falling = 1,
    Both    = 2
};


// SYSCFG port-source values (4-bit field written into EXTICR[n])
// Each pin number maps to exactly one EXTI line of the same number.
// The port-source tells SYSCFG which GPIO port owns that line.

enum class SYSCFG_EXTICR : u32 {
    PA = 0b0000, PB = 0b0001, PC = 0b0010, PD = 0b0011, PE = 0b0100,
    PF = 0b0101, PG = 0b0110
};


class Exti {
public:

    explicit Exti(Pin pin, SYSCFG_EXTICR portSrc)
        : pin(pin), portSrc(portSrc), callback(nullptr) {}

    void init()
    {
        __disable_irq();


        rcc.enableAhb1(pin.rccMask);
        rcc.enableApb2(RCC_APB2ENR::RCC_SYSCFG);

        // Configure pin as input (no pull — assumes external pull-up/down
        // on the tactile button PCB; change to Pull::Up if needed)
        pin.gpio->setMode(pin.pin, Mode::Input);
        pin.gpio->setPull(pin.pin, Pull::Up);   // active-low button typical
    }

    void configureExtiSource(ExtiTrigger trigger = ExtiTrigger::Rising)
    {
        const u8 line = pin.pin;
        // SYSCFG->EXTICR[0]  covers lines  0– 3  (4 bits each, pins 0-3)
        // SYSCFG->EXTICR[1]  covers lines  4– 7
        // SYSCFG->EXTICR[2]  covers lines  8–11
        // SYSCFG->EXTICR[3]  covers lines 12–15
        //
        // Within each register: line n uses bits [(n%4)*4 + 3 : (n%4)*4]
        {
            u32 regIndex  = line / 4;          // which of the 4 registers
            u32 bitOffset = (line % 4) * 4;    // bit position inside register
            u32 mask      = 0xFU << bitOffset;

            SYSCFG->EXTICR[regIndex] =
                (SYSCFG->EXTICR[regIndex] & ~mask) |
                (static_cast<u32>(portSrc) << bitOffset);
        }

        if (trigger == ExtiTrigger::Rising || trigger == ExtiTrigger::Both) {
            EXTI->RTSR |=  (1U << line);
        } else {
            EXTI->RTSR &= ~(1U << line);
        }

        if (trigger == ExtiTrigger::Falling || trigger == ExtiTrigger::Both) {
            EXTI->FTSR |=  (1U << line);
        } else {
            EXTI->FTSR &= ~(1U << line);
        }

        EXTI->IMR |= (1U << line);


        IRQn_Type irqn = getIrqn(line);
        NVIC_SetPriority(irqn, 5);   // mid-level priority; adjust as needed
        NVIC_EnableIRQ(irqn);
    }

    // setCallback()
    // Store a function pointer that handleIrq() will call when the line fires.

    void setCallback(void (*cb)())
    {
        callback = cb;
    }

    // handleIrq()
    // Call this from the relevant EXTIx_IRQHandler (or EXTI9_5 / EXTI15_10).
    // Clears the pending bit and invokes the callback if one is registered.
    //
    // The STM32 pending register is write-1-to-clear.

    void handleIrq()
    {
        const u32 line = pin.pin;

        if (EXTI->PR & (1U << line)) {
            EXTI->PR = (1U << line);    // clear pending (write 1 to clear)
            if (callback) {
                callback();
            }
        }
    }

    // disable() / enable()
    // Mask/unmask the line at runtime without touching the NVIC.
    // Useful for software debounce: disable on press, re-enable after delay.

    void disable()
    {
        EXTI->IMR &= ~(1U << pin.pin);
    }

    void enable()
    {
        EXTI->IMR |= (1U << pin.pin);
    }

private:
    Pin const        pin;
    SYSCFG_EXTICR    portSrc;
    void             (*callback)();

    static IRQn_Type getIrqn(u8 line)
    {
        if (line == 0)  return EXTI0_IRQn;
        if (line == 1)  return EXTI1_IRQn;
        if (line == 2)  return EXTI2_IRQn;
        if (line == 3)  return EXTI3_IRQn;
        if (line == 4)  return EXTI4_IRQn;
        if (line <= 9)  return EXTI9_5_IRQn;
        return EXTI15_10_IRQn;
    }
};

#endif /* EXTI_HPP_ */


#ifndef SYSTICK_HPP_
#define SYSTICK_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"

// Clock assumption:
//   init() reads the current HCLK from SystemCoreClock (CMSIS global).
//   If you later change the PLL, call init() again.

class Tick {
public:

    static void init()
    {

        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT       = 0;
        // DWT->CTRL bit 0 = CYCCNTENA
        DWT->CTRL        |= 1U;

        // SysTick (for millisecond tick counter)
        // SysTick_Config() is a CMSIS helper that:
        //   - sets reload = (SystemCoreClock / 1000) - 1   → 1 ms period
        //   - sets priority, clears counter, enables IRQ + counter
        SysTick_Config(SystemCoreClock / 1000U);

        tickMs = 0;
    }

    // Maximum delay: ~89 seconds at 48 MHz, ~26 seconds at 180 MHz.

    static void delayUs(u32 us)
    {
        u32 start  = DWT->CYCCNT;
        u32 cycles = us * (SystemCoreClock / 1000000U);
        while ((DWT->CYCCNT - start) < cycles) {}
    }

    // delayMs()
    // Blocking millisecond delay built on delayUs.
    // For longer waits prefer getTick() to avoid blocking the main loop.

    static void delayMs(u32 ms)
    {
        delayUs(ms * 1000U);
    }

    // getTick()
    // Returns the millisecond counter. Wraps at 2^32 (~49 days).

    static u32 getTick()
    {
        return tickMs;
    }

    // tickIsr()
    // Call this from SysTick_Handler in isr.cpp / main.cpp:
    //
    //   extern "C" void SysTick_Handler() { SysTick::tickIsr(); }

    static void tickIsr()
    {
        tickMs++;
    }

    // elapsed()
    // Helper for non-blocking timeout checks:
    //   u32 start = SysTick::getTick();
    //   if (SysTick::elapsed(start) >= 300000) { // 5 minutes }
    // Handles the 32-bit wrap correctly

    static u32 elapsed(u32 startMs)
    {
        return getTick() - startMs;
    }

private:
    static volatile u32 tickMs;
};

// Defined in systick.cpp
// volatile u32 SysTick::tickMs = 0;

#endif /* SYSTICK_HPP_ */

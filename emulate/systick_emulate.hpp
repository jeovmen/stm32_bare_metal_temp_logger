/*
 * systick_emulate.hpp
 *
 * Host-side implementation of the Tick class.
 * Replaces DWT cycle counter and SysTick ISR with std::chrono.
 * Included by main_emulate.cpp instead of systick.hpp.
 *
 *  Created on: Jun 3, 2026
 *      Author: jeovmen
 */

#ifndef SYSTICK_EMULATE_HPP_
#define SYSTICK_EMULATE_HPP_

#include "hal_mock.hpp"
#include <chrono>
#include <thread>

// Tick — host emulation version
// getTick() returns real wall-clock milliseconds since program start.
// delayUs() uses std::this_thread::sleep_for (not cycle-accurate but fine
// for FSM logic testing — DHT22 timing is not tested in emulation).
class Tick {
public:
    static void init()
    {
        startTime = std::chrono::steady_clock::now();
    }

    static u32 getTick()
    {
        auto now = std::chrono::steady_clock::now();
        return static_cast<u32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - startTime).count());
    }

    static void delayUs(u32 us)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

    static void delayMs(u32 ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    static u32 elapsed(u32 startMs)
    {
        return getTick() - startMs;
    }

    // tickIsr() not used in emulation — getTick() uses wall clock directly
    static void tickIsr() {}

private:
    static inline std::chrono::steady_clock::time_point startTime{};
};

#endif /* SYSTICK_EMULATE_HPP_ */


#ifndef FSM_HPP_
#define FSM_HPP_

#include "types.hpp"
#include "gpio.hpp"
#include "dht22.hpp"
#include "logger.hpp"
#include "systick.hpp"

enum class State : u8 {
    IDLE,
    SAMPLE
};

class Fsm {
public:

    Fsm(Pin led1, Pin led2, Dht22& dht, Logger& logger)
        : led1(led1), led2(led2), dht(dht), logger(logger),
          currentState(State::IDLE), pendingTransition(false),
          lastSampleMs(0) {}

    // init()
    // Configure LED GPIO pins as outputs and enter IDLE state.
    void init()
    {
        rcc.enableAhb1(led1.rccMask);
        rcc.enableAhb1(led2.rccMask);
        led1.gpio->setMode(led1.pin, Mode::Output);
        led2.gpio->setMode(led2.pin, Mode::Output);

        enterIdle();
    }

    // run()
    // Call repeatedly from the main loop (while(1)).
    // Handles state transitions and timed sampling.
    void run()
    {
        // Handle button press — transition between states
        if (pendingTransition) {
            pendingTransition = false;
            if (currentState == State::IDLE) {
                enterSample();
            } else {
                enterIdle();
            }
        }

        // In SAMPLE state: log every 5 minutes
        if (currentState == State::SAMPLE) {
            // 5 minutes = 300,000 ms
            // On first entry lastSampleMs is set to getTick() so the first
            // sample fires immediately after 5 minutes, not instantly.
            if (Tick::elapsed(lastSampleMs) >= SAMPLE_INTERVAL_MS) {
                lastSampleMs = Tick::getTick();
                takeSample();
            }
        }
    }

    // onButtonPress()
    // Called from EXTI ISR — sets a flag rather than acting directly,
    // so the actual transition happens safely in the main loop via run().
    void onButtonPress()
    {
        pendingTransition = true;
    }

    State getState() const { return currentState; }

private:
    static constexpr u32 SAMPLE_INTERVAL_MS = 300000U;  // 5 minutes

    Pin    led1;
    Pin    led2;
    Dht22& dht;
    Logger& logger;

    State            currentState;
    volatile bool    pendingTransition;  // set by ISR, cleared by run()
    u32              lastSampleMs;

    void enterIdle()
    {
        currentState = State::IDLE;
        setLed(led1, true);
        setLed(led2, false);
    }

    void enterSample()
    {
        currentState  = State::SAMPLE;
        lastSampleMs  = Tick::getTick();
        setLed(led1, true);
        setLed(led2, true);
    }

    void takeSample()
    {
        Dht22::Reading r = dht.read();
        if (r.valid) {
            logger.log(r);
        }
        // If read fails (e.g. sensor not ready), silently skip this tick.
        // The next interval will try again.
    }

    // setLed() — write to ODR via the Pin's gpio pointer
    void setLed(const Pin& led, bool on)
    {
        if (on) {
            led.gpio->getPort()->ODR |=  (1U << led.pin);
        } else {
            led.gpio->getPort()->ODR &= ~(1U << led.pin);
        }
    }
};

#endif /* FSM_HPP_ */



#ifndef FSM_HPP_
#define FSM_HPP_

#include "types.hpp"
#include "gpio.hpp"
#include "uart.hpp"
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

    void init()
    {
        rcc.enableAhb1(led1.rccMask);
        rcc.enableAhb1(led2.rccMask);
        led1.gpio->setMode(led1.pin, Mode::Output);
        led2.gpio->setMode(led2.pin, Mode::Output);
        enterIdle();
    }

    void run()
    {
        if (pendingTransition) {
            pendingTransition = false;
            if (currentState == State::IDLE) enterSample();
            else                              enterIdle();
        }

        if (currentState == State::SAMPLE) {
            if (Tick::elapsed(lastSampleMs) >= SAMPLE_INTERVAL_MS) {
                lastSampleMs = Tick::getTick();
                takeSample();
            }
        }
    }

    void onButtonPress()
    {
        pendingTransition = true;
    }

    State getState() const { return currentState; }

private:
    static constexpr u32 SAMPLE_INTERVAL_MS = 300000U;  // 5 minutes

    Pin     led1;
    Pin     led2;
    Dht22&  dht;
    Logger& logger;

    State         currentState;
    volatile bool pendingTransition;
    u32           lastSampleMs;

    void enterIdle()
    {
        currentState = State::IDLE;
        setLed(led1, true);
        setLed(led2, false);
        Uart::println("[FSM] -> IDLE   (LED1=ON LED2=OFF)");
    }

    void enterSample()
    {
        currentState  = State::SAMPLE;
        lastSampleMs  = Tick::getTick();
        setLed(led1, true);
        setLed(led2, true);
        Uart::println("[FSM] -> SAMPLE (LED1=ON LED2=ON)");
        Uart::println("[FSM] Next sample in 5 minutes.");
    }

    void takeSample()
    {
        Uart::print("[FSM] Sampling... tick=");
        Uart::printU32(Tick::getTick());
        Uart::println("ms");

        Dht22::Reading r = dht.read();
        if (r.valid) {
            Uart::print("[DHT22] Temp=");
            Uart::printU32(static_cast<u32>(r.temperatureC() * 10));
            Uart::print("/10 C  Humidity=");
            Uart::printU32(r.rawHumidity);
            Uart::println("/10 %RH");
            logger.log(r);
            Uart::println("[Logger] Row written to SD card.");
        } else {
            Uart::println("[DHT22] Read failed — skipping this sample.");
        }
    }

    void setLed(const Pin& led, bool on)
    {
        if (on) led.gpio->getPort()->ODR |=  (1U << led.pin);
        else    led.gpio->getPort()->ODR &= ~(1U << led.pin);
    }
};

#endif /* FSM_HPP_ */


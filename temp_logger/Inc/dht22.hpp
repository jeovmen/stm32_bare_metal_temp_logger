

#ifndef DHT22_HPP_
#define DHT22_HPP_

#include "types.hpp"
#include "gpio.hpp"
#include "systick.hpp"

// DHT22 (AM2302) single-wire driver
//
// Protocol summary (see timing diagram):
//   1. MCU pulls data line LOW for >1 ms  (start signal)
//   2. MCU releases line HIGH, switches to input
//   3. Sensor pulls LOW 80 µs, then HIGH 80 µs  (ACK)
//   4. Sensor sends 40 bits, MSB first:
//        each bit starts with 50 µs LOW preamble, then:
//        HIGH ~26-28 µs -> bit = 0
//        HIGH ~70 µs    -> bit = 1
//   5. Data layout:
//        bits [39:24] = relative humidity    (×0.1 %RH)
//        bits [23:8]  = temperature          (×0.1 °C, bit 23 = sign)
//        bits [7:0]   = checksum             (sum of the 4 data bytes)
// Usage:
//   Pin dhtPin = { &gpioA, 1, RCC_AHB1ENR::RCC_GPIOA };
//   Dht22 dht(dhtPin);
//   dht.init();                   // once in setup
//
//   Dht22::Reading r = dht.read();
//   if (r.valid) {
//       float temp = r.temperatureC();  // e.g. 23.5
//       float rh   = r.humidity();      // e.g. 58.2
//   }

class Dht22 {
public:

    struct Reading {
        bool  valid;       // false if timeout or checksum fail
        i16   rawTemp;     // signed, ×10 (e.g. 235 = 23.5 °C)
        u16   rawHumidity; // unsigned, ×10 (e.g. 582 = 58.2 %RH)

        float temperatureC() const { return rawTemp    / 10.0f; }
        float humidity()     const { return rawHumidity / 10.0f; }
    };

    explicit Dht22(Pin pin)
        : pin(pin) {}

    void init()
    {
        rcc.enableAhb1(pin.rccMask);
        setHigh();  // idle state — line pulled high by external resistor
        pin.gpio->setMode(pin.pin, Mode::Output);
        pin.gpio->setSpeed(pin.pin, Speed::Fast);
    }

    // Blocks for approximately 4–5 ms total.
    // Must not be called more often than once every 2 seconds (sensor limit).
    Reading read()
    {
        Reading result{false, 0, 0};
        u8 data[5] = {0};

        // Step 1: MCU start signal — pull low >1 ms
        pin.gpio->setMode(pin.pin, Mode::Output);
        setLow();
        Tick::delayMs(2);   // 2 ms, safely above the 1 ms minimum

        // Step 2: Release line, switch to input
        setHigh();
        pin.gpio->setMode(pin.pin, Mode::Input);
        pin.gpio->setPull(pin.pin, Pull::None);  // rely on external pull-up
        Tick::delayUs(40);  // wait for sensor to take over

        // Step 3: Read sensor ACK (80 µs low, 80 µs high)
        if (!waitForLevel(0, 100)) return result;  // expect LOW ACK
        if (!waitForLevel(1, 100)) return result;  // expect HIGH ACK

        // Step 4: Read 40 data bits
        for (int i = 0; i < 40; i++) {
            // Each bit starts with a ~50 µs LOW preamble
            if (!waitForLevel(0, 100)) return result;

            // Time how long the HIGH phase lasts — determines 0 or 1
            u32 highUs = measureHighUs(100);
            if (highUs == 0) return result;  // timeout

            data[i / 8] <<= 1;
            if (highUs > 40) {
                data[i / 8] |= 1;  // HIGH > 40 µs -> bit 1
            }
            // HIGH <= 40 µs -> bit 0 (already 0 from shift)
        }

        // Step 5: Verify checksum
        u8 checksum = data[0] + data[1] + data[2] + data[3];
        if (checksum != data[4]) return result;

        // Step 6: Decode humidity and temperature
        result.rawHumidity = (static_cast<u16>(data[0]) << 8) | data[1];

        // Temperature: bit 15 of the 16-bit word is the sign bit
        u16 rawT = (static_cast<u16>(data[2]) << 8) | data[3];
        if (rawT & 0x8000) {
            result.rawTemp = -static_cast<i16>(rawT & 0x7FFF);
        } else {
            result.rawTemp =  static_cast<i16>(rawT);
        }

        // Restore pin to idle output-high for next transaction
        pin.gpio->setMode(pin.pin, Mode::Output);
        setHigh();

        result.valid = true;
        return result;
    }

private:
    Pin const pin;

    void setLow()
    {
        pin.gpio->getPort()->ODR &= ~(1U << pin.pin);
    }

    void setHigh()
    {
        pin.gpio->getPort()->ODR |= (1U << pin.pin);
    }

    u8 readPin()
    {
        return (pin.gpio->getPort()->IDR >> pin.pin) & 1U;
    }

    bool waitForLevel(u8 level, u32 timeoutUs)
    {
        u32 start  = DWT->CYCCNT;
        u32 limit  = timeoutUs * (SystemCoreClock / 1000000U);
        while (readPin() != level) {
            if ((DWT->CYCCNT - start) >= limit) return false;
        }
        return true;
    }

    u32 measureHighUs(u32 timeoutUs)
    {
        u32 start  = DWT->CYCCNT;
        u32 limit  = timeoutUs * (SystemCoreClock / 1000000U);
        while (readPin() == 1) {
            if ((DWT->CYCCNT - start) >= limit) return 0;
        }
        // Convert elapsed cycles back to microseconds
        return (DWT->CYCCNT - start) / (SystemCoreClock / 1000000U);
    }
};

#endif /* DHT22_HPP_ */

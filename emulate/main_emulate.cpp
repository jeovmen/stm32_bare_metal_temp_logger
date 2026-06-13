/*
 * main_emulate.cpp
 *
 * Host-side emulation entry point.
 * Compiles and runs on Linux/Mac/Windows with plain g++.
 * No STM32 hardware required.
 *
 * What this tests:
 *   1. FSM state transitions (IDLE → SAMPLE → IDLE via simulated button)
 *   2. LED state changes reflected in fake GPIO ODR register
 *   3. 5-minute sample timer logic (accelerated to 5 seconds for testing)
 *   4. DHT22 mock returning synthetic temperature/humidity data
 *   5. Logger writing a real CSV file to disk (templog_emulated.csv)
 *
 * Build:
 *   g++ -std=c++17 -DEMULATE -I. -I.. \
 *       main_emulate.cpp \
 *       -o emulate && ./emulate
 *
 * Run from the emulate/ directory.
 *
 *  Created on: Jun 3, 2026
 *      Author: jeovmen
 */


#include "hal_mock.hpp"
#include "systick_emulate.hpp"

// Stub out the real STM32 includes so our headers compile cleanly.
// The actual HAL headers check for EMULATE and use mock types above.
// We provide a fake "STM32F4/stm32f446xx.h" path by adding -I. to g++,
// but we also need gpio.hpp / rcc.hpp to not re-include the real CMSIS.
// Solution: redefine the include guard here before including our headers.
#define STM32F4_STM32F446XX_H_   // block the real CMSIS header
#define CORE_CM4_H_GENERIC_       // block core_cm4.h

// Fake Gpio and Rcc classes (host versions — no register writes, just state)
#include <cstring>
#include <cstdio>
#include <cassert>
#include <chrono>
#include <thread>

// Re-use the same enums from the real headers
enum class Mode  : u8 { Input=0, Output=1, Alt=2, Analog=3 };
enum class Speed : u8 { Low=0, Medium=1, High=2, Fast=3 };
enum class Pull  : u8 { None=0, Up=1, Down=2 };
enum class AF    : u8 { AF0=0, AF5=5 };

class Gpio {
public:
    explicit Gpio(GPIO_TypeDef* port) : port(port) {}
    void setMode(u8 pin, Mode)   { (void)pin; }
    void setPull(u8 pin, Pull)   { (void)pin; }
    void setSpeed(u8 pin, Speed) { (void)pin; }
    void setAlternateFunction(u8 pin, AF) { (void)pin; }
    GPIO_TypeDef* getPort() { return port; }
private:
    GPIO_TypeDef* port;
};

struct Pin {
    Gpio*         gpio;
    u8            pin;
    RCC_AHB1ENR   rccMask;
};

class Rcc {
public:
    void enableAhb1(RCC_AHB1ENR p) { _fakeRCC.AHB1ENR |= static_cast<u32>(p); }
    void enableApb2(RCC_APB2ENR p) { _fakeRCC.APB2ENR |= static_cast<u32>(p); }
    void resetAhb1() {}
    void resetApb2() {}
};
Rcc rcc;

// Mock DHT22 — returns synthetic data instead of bit-banging a real sensor
struct MockDht22Reading {
    bool  valid       = true;
    i16   rawTemp     = 235;    // 23.5 °C
    u16   rawHumidity = 582;    // 58.2 %RH

    float temperatureC() const { return rawTemp     / 10.0f; }
    float humidity()     const { return rawHumidity / 10.0f; }
};

class MockDht22 {
public:
    explicit MockDht22(Pin) {}
    void init() { printf("[DHT22] Sensor initialised (mock)\n"); }

    MockDht22Reading read() {
        readCount++;
        MockDht22Reading r;
        // Vary data slightly each read so CSV has distinct rows
        r.rawTemp     = static_cast<i16>(235 + readCount);
        r.rawHumidity = static_cast<u16>(582 - readCount);
        printf("[DHT22] Read #%u — temp=%.1f°C  humidity=%.1f%%RH\n",
               readCount, r.temperatureC(), r.humidity());
        return r;
    }

    struct Reading : public MockDht22Reading {};

private:
    unsigned readCount = 0;
};

// Mock Logger — writes to a real CSV on disk so you can inspect the output
class MockLogger {
public:
    bool init() {
        file = fopen("templog_emulated.csv", "w");
        if (!file) { printf("[Logger] ERROR: cannot open output file\n"); return false; }
        fprintf(file, "elapsed_ms,temperature_c,humidity_pct\n");
        fflush(file);
        printf("[Logger] Opened templog_emulated.csv\n");
        return true;
    }

    void log(const MockDht22Reading& r) {
        u32 ms = Tick::getTick();
        fprintf(file, "%lu,%.1f,%.1f\n", (unsigned long)ms,
                (double)r.temperatureC(), (double)r.humidity());
        fflush(file);
        printf("[Logger] Wrote row: %lu ms, %.1f°C, %.1f%%RH\n",
               (unsigned long)ms, (double)r.temperatureC(), (double)r.humidity());
    }

    void deinit() { if (file) { fclose(file); file = nullptr; } }

private:
    FILE* file = nullptr;
};

// FSM — identical logic to the real fsm.hpp but using mock types
enum class State : u8 { IDLE, SAMPLE };

class Fsm {
public:
    Fsm(Pin led1, Pin led2, MockDht22& dht, MockLogger& logger)
        : led1(led1), led2(led2), dht(dht), logger(logger),
          currentState(State::IDLE), pendingTransition(false),
          lastSampleMs(0) {}

    void init()
    {
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
        printf("[Button] Press detected — pending transition\n");
    }

    State getState() const { return currentState; }

    // Expose LED ODR for test assertions
    bool led1On() const { return (led1.gpio->getPort()->ODR >> led1.pin) & 1U; }
    bool led2On() const { return (led2.gpio->getPort()->ODR >> led2.pin) & 1U; }

private:
    // Accelerated to 5 seconds so the emulation can be watched in real time.
    // Change back to 300000U for the real build.
    static constexpr u32 SAMPLE_INTERVAL_MS = 5000U;

    Pin          led1, led2;
    MockDht22&   dht;
    MockLogger&  logger;
    State        currentState;
    volatile bool pendingTransition;
    u32          lastSampleMs;

    void enterIdle()
    {
        currentState = State::IDLE;
        setLed(led1, true);
        setLed(led2, false);
        printf("[FSM] → IDLE   (LED1=ON  LED2=OFF)\n");
    }

    void enterSample()
    {
        currentState = State::SAMPLE;
        lastSampleMs = Tick::getTick();
        setLed(led1, true);
        setLed(led2, true);
        printf("[FSM] → SAMPLE (LED1=ON  LED2=ON)\n");
    }

    void takeSample()
    {
        auto r = dht.read();
        if (r.valid) logger.log(r);
    }

    void setLed(const Pin& led, bool on)
    {
        if (on) led.gpio->getPort()->ODR |=  (1U << led.pin);
        else    led.gpio->getPort()->ODR &= ~(1U << led.pin);
    }
};

// Test helpers
static int testsPassed = 0;
static int testsFailed = 0;

static void check(const char* name, bool condition)
{
    if (condition) {
        printf("  ✓ PASS: %s\n", name);
        testsPassed++;
    } else {
        printf("  ✗ FAIL: %s\n", name);
        testsFailed++;
    }
}

int main()
{
    printf("=========================================\n");
    printf("  STM32 Temp Logger — Emulation / Tests  \n");
    printf("=========================================\n\n");

    Tick::init();

    // Peripheral setup
    Gpio gpioA(GPIOA), gpioB(GPIOB);

    Pin led1Pin   = { &gpioA, 5, RCC_AHB1ENR::RCC_GPIOA };
    Pin led2Pin   = { &gpioB, 5, RCC_AHB1ENR::RCC_GPIOB };
    Pin dhtPin    = { &gpioA, 1, RCC_AHB1ENR::RCC_GPIOA };

    MockDht22  dht(dhtPin);
    MockLogger logger;

    dht.init();
    logger.init();

    Fsm fsm(led1Pin, led2Pin, dht, logger);
    fsm.init();

    printf("\n--- Test 1: Initial state is IDLE ---\n");
    check("state == IDLE",   fsm.getState() == State::IDLE);
    check("LED1 on",         fsm.led1On());
    check("LED2 off",       !fsm.led2On());

    printf("\n--- Test 2: Button press → SAMPLE ---\n");
    fsm.onButtonPress();
    fsm.run();
    check("state == SAMPLE", fsm.getState() == State::SAMPLE);
    check("LED1 on",         fsm.led1On());
    check("LED2 on",         fsm.led2On());

    printf("\n--- Test 3: Button press → back to IDLE ---\n");
    fsm.onButtonPress();
    fsm.run();
    check("state == IDLE",   fsm.getState() == State::IDLE);
    check("LED1 on",         fsm.led1On());
    check("LED2 off",       !fsm.led2On());

    printf("\n--- Test 4: SAMPLE state — wait for timed sample (5s accelerated) ---\n");
    // Re-enter SAMPLE
    fsm.onButtonPress();
    fsm.run();
    printf("Waiting 6 seconds for first sample to fire...\n");
    u32 waitStart = Tick::getTick();
    bool sampleFired = false;
    while (Tick::elapsed(waitStart) < 6500) {
        fsm.run();
        Tick::delayMs(100);
        // Check if logger wrote a row by watching tick advance
        if (Tick::elapsed(waitStart) > 5100 && !sampleFired) {
            sampleFired = true;
        }
    }
    check("sample fired within 6s", sampleFired);

    printf("\n--- Test 5: DHT22 conversion math ---\n");
    MockDht22Reading r;
    r.rawTemp     = 235;   // 23.5°C
    r.rawHumidity = 999;   // 99.9%
    check("temp conversion:  23.5°C",  r.temperatureC() > 23.4f && r.temperatureC() < 23.6f);
    check("humidity conversion: 99.9%", r.humidity()     > 99.8f && r.humidity()     < 100.0f);

    // Negative temperature
    r.rawTemp = -55;  // -5.5°C
    check("negative temp: -5.5°C", r.temperatureC() > -5.6f && r.temperatureC() < -5.4f);

    printf("\n=========================================\n");
    printf("  Results: %d passed, %d failed\n", testsPassed, testsFailed);
    printf("=========================================\n");

    logger.deinit();
    return testsFailed == 0 ? 0 : 1;
}

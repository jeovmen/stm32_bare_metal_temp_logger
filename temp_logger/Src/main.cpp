
#include "gpio.hpp"
#include "rcc.hpp"
#include "spi.hpp"
#include "exti.hpp"
#include "systick.hpp"
#include "uart.hpp"
#include "dht22.hpp"
#include "logger.hpp"
#include "fsm.hpp"

Gpio gpioA(GPIOA);
Gpio gpioB(GPIOB);
Gpio gpioC(GPIOC);

Rcc rcc;

// LED pins
// LED1: PA8 (moved off PA5 to avoid SPI1 SCK conflict)
// LED2: PB5 external LED
Pin led1Pin = { &gpioA, 8, RCC_AHB1ENR::RCC_GPIOA };
Pin led2Pin = { &gpioB, 5, RCC_AHB1ENR::RCC_GPIOB };

// Blue user button — PC13, falling edge
Pin buttonPin = { &gpioC, 13, RCC_AHB1ENR::RCC_GPIOC };
Exti buttonExti(buttonPin, SYSCFG_EXTICR::PC);

// DHT22 — PA1 (requires 4.7 kΩ pull-up to 3.3 V)
Pin dhtPin = { &gpioA, 1, RCC_AHB1ENR::RCC_GPIOA };
Dht22 dht(dhtPin);

// SPI1 for SD card — PA5=SCK, PA6=MISO, PA7=MOSI, PA9=CS
Pin sckPin  = { &gpioA, 5, RCC_AHB1ENR::RCC_GPIOA };
Pin misoPin = { &gpioA, 6, RCC_AHB1ENR::RCC_GPIOA };
Pin mosiPin = { &gpioA, 7, RCC_AHB1ENR::RCC_GPIOA };
Pin nssPin  = { &gpioA, 9, RCC_AHB1ENR::RCC_GPIOA };

SpiPins sdPins  = { sckPin, misoPin, mosiPin, &nssPin };
Spi     sdSpi(SPI1, sdPins);

// Logger and FSM
Logger logger;
Fsm    fsm(led1Pin, led2Pin, dht, logger);

// ISR handlers
extern "C" void SysTick_Handler()
{
    Tick::tickIsr();
}

extern "C" void EXTI15_10_IRQHandler()
{
    buttonExti.handleIrq();
}

int main(void)
{
    // 1. UART first so we can see debug output from the start
    Uart::init();
    Uart::println("=== STM32 Temp Logger ===");
    Uart::println("Initializing...");

    // 2. Tick timer
    Tick::init();
    Uart::println("Tick: OK");

    // 3. SPI + SD card
    sdSpi.initGpio();
    sdSpi.config();
    Uart::println("SPI: OK");

    // 4. DHT22
    dht.init();
    Uart::println("DHT22: OK");

    // 5. Logger
    if (logger.init()) {
        Uart::println("Logger: OK");
    } else {
        Uart::println("Logger: FAILED (no SD card?)");
    }

    // 6. Button EXTI
    buttonExti.init();
    buttonExti.configureExtiSource(ExtiTrigger::Falling);
    buttonExti.setCallback([]{ fsm.onButtonPress(); });
    Uart::println("EXTI: OK");

    // 7. FSM
    fsm.init();
    Uart::println("FSM: OK -- entering IDLE");
    Uart::println("Press button to toggle SAMPLE mode.");

    // 8. Enable interrupts
    __enable_irq();

    // 9. Main loop
    while (1)
    {
        fsm.run();
    }
}

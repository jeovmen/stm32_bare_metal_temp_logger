
#include "gpio.hpp"
#include "rcc.hpp"
#include "spi.hpp"
#include "exti.hpp"
#include "systick.hpp"
#include "dht22.hpp"
#include "logger.hpp"
#include "fsm.hpp"

Gpio gpioA(GPIOA);
Gpio gpioB(GPIOB);
Gpio gpioC(GPIOC);

Rcc rcc;

Pin led1Pin = { &gpioA, 5,  RCC_AHB1ENR::RCC_GPIOA };   // onboard LD2
Pin led2Pin = { &gpioB, 5,  RCC_AHB1ENR::RCC_GPIOB };   // external LED

Pin buttonPin = { &gpioC, 13, RCC_AHB1ENR::RCC_GPIOC };
Exti buttonExti(buttonPin, SYSCFG_EXTICR::PC);

Pin dhtPin = { &gpioA, 1, RCC_AHB1ENR::RCC_GPIOA };
Dht22 dht(dhtPin);

Pin sckPin  = { &gpioA, 5, RCC_AHB1ENR::RCC_GPIOA };
Pin misoPin = { &gpioA, 6, RCC_AHB1ENR::RCC_GPIOA };
Pin mosiPin = { &gpioA, 7, RCC_AHB1ENR::RCC_GPIOA };
Pin nssPin  = { &gpioA, 9, RCC_AHB1ENR::RCC_GPIOA };

SpiPins sdPins = { sckPin, misoPin, mosiPin, &nssPin };
Spi sdSpi(SPI1, sdPins);   // extern'd by diskio.cpp


Logger logger;
Fsm    fsm(led1Pin, led2Pin, dht, logger);


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
    // 1. Tick timer (must come first — DHT22 and logger depend on it)
    Tick::init();

    // 2. SPI + SD card
    sdSpi.initGpio();
    sdSpi.config();

    // 3. DHT22
    dht.init();

    // 4. Logger (mounts FatFs, opens CSV file)
    logger.init();

    // 5. Button EXTI
    buttonExti.init();
    buttonExti.configureExtiSource(ExtiTrigger::Falling);
    buttonExti.setCallback([]{ fsm.onButtonPress(); });

    // 6. FSM (configures LED GPIOs, enters IDLE)
    fsm.init();

    // 7. Enable interrupts
    __enable_irq();

    // 8. Main loop
    while (1)
    {
        fsm.run();
    }
}

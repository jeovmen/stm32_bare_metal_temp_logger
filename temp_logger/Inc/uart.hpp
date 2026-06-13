
#ifndef UART_HPP_
#define UART_HPP_

#include "types.hpp"
#include "STM32F4/stm32f446xx.h"
#include "gpio.hpp"
#include "rcc.hpp"


class Uart {
public:

    // init()
    // Configure USART2 on PA2(TX)/PA3(RX) at 115200 baud, 8N1.
    // Clock source: 16 MHz HSI (default after reset, before any PLL config).
    static void init()
    {
        // Enable clocks: GPIOA and USART2 (USART2 is on APB1)
        RCC->AHB1ENR |= (1U << 0);    // GPIOA
        RCC->APB1ENR |= (1U << 17);   // USART2

        // PA2 = TX, PA3 = RX → Alternate Function 7
        // MODER: bits [5:4] = PA2, bits [7:6] = PA3 → AF mode (0b10)
        GPIOA->MODER &= ~((3U << 4) | (3U << 6));
        GPIOA->MODER |=  ((2U << 4) | (2U << 6));

        // OSPEEDR: fast speed on PA2/PA3
        GPIOA->OSPEEDR |= ((3U << 4) | (3U << 6));

        // AFR[0]: PA2=AF7 (bits [11:8]), PA3=AF7 (bits [15:12])
        GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
        GPIOA->AFR[0] |=  ((7U  << 8) | (7U  << 12));

        // BRR: 115200 baud at 16 MHz HSI
        // BRR = fCLK / baud = 16,000,000 / 115,200 ≈ 138.88
        // Mantissa = 138 = 0x8A, Fraction = 0.88 × 16 ≈ 14 = 0xE
        // But for QEMU compatibility use the simpler integer: 0x008B (138.6875)
        USART2->BRR = 0x008B;

        // CR1: enable USART, enable transmitter
        USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
    }

    // putChar() — send one byte, block until TX register is empty
    static void putChar(char c)
    {
        while (!(USART2->SR & USART_SR_TXE)) {}
        USART2->DR = static_cast<u32>(c);
    }

    // print() — send a null-terminated string
    static void print(const char* s)
    {
        while (*s) putChar(*s++);
    }

    // println() — send string + CRLF (QEMU terminal needs \r\n)
    static void println(const char* s)
    {
        print(s);
        putChar('\r');
        putChar('\n');
    }

    // printU32() — print an unsigned integer without needing printf
    static void printU32(u32 val)
    {
        if (val == 0) { putChar('0'); return; }
        char buf[11];
        int  i = 0;
        while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
        while (i > 0)   { putChar(buf[--i]); }
    }
};

#endif /* UART_HPP_ */

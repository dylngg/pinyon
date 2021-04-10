#include "console.hpp"
#include <pine/printf.hpp>
#include <pine/string.hpp>

static inline void spin(int32_t count)
{
    asm volatile("__spin_%=: subs %[count], %[count], #1; bne __spin_%=\n"
                 : "=r"(count)
                 : [ count ] "0"(count)
                 : "cc");
}

void console_init()
{
    /*
     * This bit of code was modified from:
     * https://jsandler18.github.io/explanations/kernel_c.html
     * and partially derived from the BCM2835 ARM Peripherals manual.
     */

    // Reset UART
    *(volatile size_t*)(UART_CR) = 0;

    /*
     * Disable the GPIO14 and GPIO15 pins (supports RXD input, TXD output). The
     * spins are required.
     *
     * See Page ~101 in manual for GPIO setup, page ~177 for UART stuff
     */

    *(volatile size_t*)(GPPUD) = 0; // we should disable something
    spin(150);

    *(volatile size_t*)(GPPUDCLK0) = (1 << GPIO14) | (1 << GPIO15); // disable this
    spin(150);

    *(volatile size_t*)(GPPUDCLK0) = 0x00000000; // ok now take effect

    // Configure UART
    *(volatile size_t*)(UART_ICR) = 0x7FF; // clear all pending interrupts; write 1s for 11 bits

    /*
     * We want a baud rate of 115200, speed is 250 MHz
     *
     * BAUDDIV = (FUARTCLK/(16 Baud rate))
     *
     * So we get 1.67. We store the integer in IBRD, and the fractional value
     * in FBRD. FBRD has a range of 0-63 (6 bits), so we take (67 / 100) * 63 = ~42.
     */
    *(volatile size_t*)(UART_IBRD) = 1;
    *(volatile size_t*)(UART_FBRD) = 42;

    // 4: enable FIFO
    // 5/6: 0b11 - 8-bit words
    // rest should be 0 to disable parity, 2-stop bit, etc
    *(volatile size_t*)(UART_LCRH) = (1 << UART_LCRH_FEN) | (1 << UART_LCRH_LWEN0) | (1 << UART_LCRH_LWEN1);

    // Reset UART so that it disables all interrupts by writing 1s
    // skip 0, 2 and 3 since those settings are unsupported.
    *(volatile size_t*)(UART_IMSC) = ~((1 << UART_IMSC_RIMIM) | (1 << UART_IMSC_DCDMIM) | (1 << UART_IMSC_DSRMIM)) & 0xFFF;

    // Enable the UART (0), enable transmit/recieve (8, 9)
    *(volatile size_t*)(UART_CR) = (1 << 0) | (1 << UART_CR_TXE) | (1 << UART_CR_RXE);
}
void console_put(char ch)
{
    // 5: TXFF bit; set when transmit FIFO is full
    while (*(volatile uint32_t*)(UART_FR) & (1 << 5)) {
    }
    *(volatile size_t*)(UART_DATA) = ch;
}

char console_get()
{
    // 4: RXFE bit; set when recieve FIFO is full
    while (*(volatile uint32_t*)(UART_FR) & (1 << 4)) {
    }
    return *(volatile size_t*)(UART_DATA);
}

void console(const StringView& message)
{
    for (auto ch : message)
        console_put(ch);
}

void consoleln(const StringView& message)
{
    console(message);
    console_put('\n');
}

void consolef(const StringView& fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const auto& try_add_wrapper = [](const StringView& message) -> bool {
        console(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}
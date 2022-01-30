#include "console.hpp"
#include "interrupts.hpp"
#include <pine/barrier.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>

void UARTManager::poll_write(const char* message)
{
    for (size_t i = 0; message[i] != '\0'; i++)
        poll_put(message[i]);
}

void UARTManager::poll_write(const char* message, size_t bytes)
{
    for (size_t i = 0; i < bytes; i++)
        poll_put(message[i]);
}

void console_readline(char* buf, size_t bufsize)
{
    MemoryBarrier barrier;
    auto& uart = uart_manager();
    UARTManager::ReadWriteInterruptMask mask(uart);

    size_t offset = 0;
    char ch;
    for (;;) {
        ch = uart.poll_get();

        if (offset >= bufsize - 1)
            continue;

        bool stop = false;
        switch (ch) {
        case '\r':
            stop = true;
            break;
        case '\n':
            stop = true;
            break;
        case '\t':
            // ignore; messes with delete :P
            continue;
        case 127:
            // delete
            if (offset >= 1) {
                offset--;
                // move left one char (D), then delete one (P)
                uart.poll_write("\x1b[1D\x1b[1P");
            }
            continue;
        default:
            // local echo
            uart.poll_put(ch);
        }
        if (stop)
            break;

        buf[offset] = ch;
        offset++;
    }

    uart.poll_put('\n');
    buf[offset] = '\0';
    return;
}

void console(const char* message)
{
    MemoryBarrier barrier;
    auto& uart = uart_manager();
    UARTManager::WriteInterruptMask mask(uart);

    uart.poll_write(message);
}

void console(const char* message, size_t bytes)
{
    MemoryBarrier barrier;
    auto& uart = uart_manager();
    UARTManager::WriteInterruptMask mask(uart);

    uart.poll_write(message, bytes);
}

void consoleln(const char* message)
{
    MemoryBarrier barrier;
    auto& uart = uart_manager();
    UARTManager::WriteInterruptMask mask(uart);

    uart.poll_write(message);
    uart.poll_put('\n');
}

void consolef(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    MemoryBarrier barrier;
    auto& uart = uart_manager();
    UARTManager::WriteInterruptMask mask(uart);

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        uart.poll_write(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}

static inline void spin(u32 count)
{
    asm volatile("__spin_%=: subs %[count], %[count], #1; bne __spin_%=\n"
                 : "=r"(count)
                 : [count] "0"(count)
                 : "cc");
}

void UARTManager::reset()
{
    MemoryBarrier barrier;

    /* Reset UART */
    cr = 0; // Reset

    /*
     * Disable the GPIO14 and GPIO15 pins (supports RXD input, TXD output). The
     * spins are required.
     *
     * See Page ~101 in manual for GPIO setup, page ~177 for UART stuff
     */

    *(volatile u32*)(GPPUD) = 0; // we should disable something
    spin(150);

    *(volatile u32*)(GPPUDCLK0) = (1 << GPIO14) | (1 << GPIO15); // disable this
    spin(150);

    *(volatile u32*)(GPPUDCLK0) = 0; // ok now take effect

    /* Configure UART */
    icr = 0x7FF; // clear all pending interrupts; write 1s for 11 bits

    /*
     * We want a baud rate of 115200, speed is 250 MHz
     *
     * BAUDDIV = (FUARTCLK/(16 Baud rate))
     *
     * So we get 1.67. We store the integer in IBRD, and the fractional value
     * in FBRD. FBRD has a range of 0-63 (6 bits), so we take (67 / 100) * 63 = ~42.
     */
    ibrd = 1;
    fbrd = 42;

    // 4: enable FIFO
    // 5/6: 0b11 - 8-bit words
    // rest should be 0 to disable parity, 2-stop bit, etc
    lcrh = (1 << UART_LCRH_FEN) | (1 << UART_LCRH_LWEN0) | (1 << UART_LCRH_LWEN1);

    /*
     * Reset UART and disable all interrupts.
     */
    imsc = 0;

    /*
     * Set the FIFO level select to 1/8 full for both transmit (0-2 bits) and
     * recieve (3-5)
     */
    ifls = 0;

    /*
     * Finally enable the UART (0) and the transmit/recieve bits (8, 9).
     */
    cr = (1 << UART_CR_EN) | (1 << UART_CR_TXE) | (1 << UART_CR_RXE);
}

void UARTManager::handle_irq()
{
}

void UARTManager::poll_put(char ch)
{
    // 5: TXFF bit; set when transmit FIFO is full
    while (fr & (1 << UART_FR_TXFF)) {
    }
    dr = ch;
}

char UARTManager::poll_get()
{
    // 4: RXFE bit; set when recieve FIFO is empty
    while (fr & (1 << UART_FR_RXFE)) {
    }
    return dr;
}

UARTManager& uart_manager()
{
    static auto* g_uart_manager = reinterpret_cast<UARTManager*>(UART_BASE);
    return *g_uart_manager;
}

void uart_init()
{
    uart_manager().reset();
    irq_manager().enable_uart();
}
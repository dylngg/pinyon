#include "console.hpp"
#include <pine/barrier.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>

static inline void console_put(char ch)
{
    uart_manager()->put(ch);
}

static inline char console_get()
{
    return uart_manager()->get();
}

void console_readline(char* buf, size_t bufsize)
{
    size_t offset = 0;
    char ch;

    for (;;) {
        ch = console_get();

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
                console("\x1b[1D\x1b[1P");
            }
            continue;
        default:
            // local echo
            console_put(ch);
        }
        if (stop)
            break;

        buf[offset] = ch;
        offset++;
    }

    console_put('\n');
    buf[offset] = '\0';
    return;
}

void console(const char* message)
{
    MemoryBarrier barrier {};
    for (size_t i = 0; message[i] != '\0'; i++)
        console_put(message[i]);
}

void console(const char* message, size_t bytes)
{
    MemoryBarrier barrier {};
    for (size_t i = 0; i < bytes; i++)
        console_put(message[i]);
}

void consoleln(const char* message)
{
    MemoryBarrier barrier {};
    console(message);
    console_put('\n');
}

void consolef(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const auto& try_add_wrapper = [](const char* message) -> bool {
        console(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}

static inline void spin(u32 count)
{
    asm volatile("__spin_%=: subs %[count], %[count], #1; bne __spin_%=\n"
                 : "=r"(count)
                 : [ count ] "0"(count)
                 : "cc");
}

void UARTManager::init() volatile
{
    MemoryBarrier barrier {};

    /* Reset UART */
    ctrl = 0; // Reset

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
    intr_clear = 0x7FF; // clear all pending interrupts; write 1s for 11 bits

    /*
     * We want a baud rate of 115200, speed is 250 MHz
     *
     * BAUDDIV = (FUARTCLK/(16 Baud rate))
     *
     * So we get 1.67. We store the integer in IBRD, and the fractional value
     * in FBRD. FBRD has a range of 0-63 (6 bits), so we take (67 / 100) * 63 = ~42.
     */
    int_baud_rate_div = 1;
    frac_baud_rate_div = 42;

    // 4: enable FIFO
    // 5/6: 0b11 - 8-bit words
    // rest should be 0 to disable parity, 2-stop bit, etc
    line_ctrl = (1 << UART_LCRH_FEN) | (1 << UART_LCRH_LWEN0) | (1 << UART_LCRH_LWEN1);

    /*
     * Reset UART so that it disables all interrupts
     *
     * This is done by writing 1s to the interrupt mask register. Skip
     * 0,2,3 because the BCM2835 doesn't support those.
     */
    intr_mask_set_clear = 0xFF2;

    /*
     * Finally enable the UART (0) and the transmit/recieve bits (8, 9).
     */
    ctrl = (1 << UART_CR_EN) | (1 << UART_CR_TXE) | (1 << UART_CR_RXE);
}

inline void UARTManager::put(char ch) volatile
{
    // 5: TXFF bit; set when transmit FIFO is full
    while (flag & (1 << 5)) {
    }
    data = ch;
}

inline char UARTManager::get() volatile
{
    // 4: RXFE bit; set when recieve FIFO is full
    while (flag & (1 << 4)) {
    }
    return data;
}

static auto* g_uart_manager = (UARTManager*)UART_BASE;

inline UARTManager* uart_manager()
{
    return g_uart_manager;
}

void uart_init()
{
    uart_manager()->init();
}

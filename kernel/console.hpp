#pragma once

#include <pine/types.hpp>

/*
 * For the Raspberry Pi 2 we are using the PL011 UART.
 *
 * See page 177 of Broadcom BCM2836 Datasheet for details
 * on these registers.
 */
#define GPIO14 14
#define GPIO15 15

#define UART_BASE 0x3F201000

#define UART_LCRH_FEN 4
#define UART_LCRH_LWEN0 5
#define UART_LCRH_LWEN1 6

#define UART_CR_EN 0
#define UART_CR_TXE 8
#define UART_CR_RXE 9

#define UART_FR_RXFE 4
#define UART_FR_TXFF 5

#define UART_IMSC_RXIM 4
#define UART_IMSC_TXIM 5

// See page 101 of Broadcom BCM2836 Datasheet
#define GPPUD 0x3F200094
#define GPPUDCLK0 0x3F200098

/*
 * Kernel functions that poll. This is meant for logging, not for userspace
 * usage (use read/write for that).
 */
void console_readline(char*, size_t);

void console(const char*);

void console(const char*, size_t);

void consoleln(const char*);

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));

struct UARTManager {
public:
    void init();
    void handle_irq();

private:
    char poll_get();
    void poll_put(char ch);
    void poll_write(const char*);
    void poll_write(const char*, size_t);

    friend void console_readline(char*, size_t);
    friend void console(const char*);
    friend void console(const char*, size_t);
    friend void consoleln(const char*);
    friend void consolef(const char*, ...);

    template <int offset1, int offset2>
    class InterruptMask {
    public:
        // Compute mask at compile time; this also helps when offset1 == offset2
        constexpr inline u32 mask() const
        {
            return (1 << offset1) | (1 << offset2);
        }
        InterruptMask(volatile UARTManager* uart) __attribute__((always_inline))
        : m_uart(uart)
        // Store prev so when the mask is already set (perhaps by a ancestor
        // InterruptMask class in the call stack), we can put it in the
        // previous state we found it in.
        , m_prev(uart->imsc)
        {
            m_uart->imsc = m_prev & ~mask();
        }

        ~InterruptMask() __attribute__((always_inline))
        {
            m_uart->imsc |= m_prev & mask();
        }

    private:
        volatile UARTManager* m_uart;
        const u8 m_prev;
    };

    using ReadInterruptMask = InterruptMask<UART_IMSC_RXIM, UART_IMSC_RXIM>;
    using WriteInterruptMask = InterruptMask<UART_IMSC_TXIM, UART_IMSC_TXIM>;
    using ReadWriteInterruptMask = InterruptMask<UART_IMSC_RXIM, UART_IMSC_TXIM>;

    /*
     * See section 13.3 on page 176 in the BCM2835 manual for the
     * definition of these.
     *
     * Most are not used here and not all registers are supported
     * by broadcom (since the BCM2835's UART is based on PrimeCell's
     * PL011).
     *
     * See also the PrimeCell UART (PL011) Manual:
     * https://developer.arm.com/documentation/ddi0183/f/
     */
    volatile u32 dr; // DR
    volatile u32 rsrecr; // RSRECR
    volatile u32 unused1;
    volatile u32 unused2;
    volatile u32 unused3;
    volatile u32 unused4;
    volatile u32 fr; // FR
    volatile u32 ibpr; // IBPR
    volatile u32 ilrd; // ILPR
    volatile u32 ibrd; // IBRD
    volatile u32 fbrd; // FBRD
    volatile u32 lcrh; // LCRH
    volatile u32 cr; // CR
    volatile u32 ifls; // IFLS
    volatile u32 imsc; // IMSC
    volatile u32 ris; // RIS
    volatile u32 mis; // MIS
    volatile u32 icr; // ICR
    volatile u32 dmacr; // DMACR
};

UARTManager* uart_manager();

void uart_init();

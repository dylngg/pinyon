#pragma once
#include "interrupts.hpp"
#include "kmalloc.hpp"

#include <pine/maybe.hpp>
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

#define UART_ICR_RXIC 4
#define UART_ICR_TXIC 5

// See page 101 of Broadcom BCM2836 Datasheet
#define GPPUD 0x3F200094
#define GPPUDCLK0 0x3F200098

class UARTResource {
public:
    struct Options {
        bool is_write = false;
        bool local_echo_on_read = !is_write;
    };

    ~UARTResource();

    /*
     * Returns the resource for async reading if an existing request is not in
     * progress.
     */
    static Maybe<KOwner<UARTResource>> try_request(char*, size_t, Options);
    static void handle_irq(InterruptsDisabledTag);

    size_t size() const { return m_size; }
    size_t amount_left() const { return m_capacity - m_size; }
    bool is_finished() const { return amount_left() == 0; }
    bool is_write_request() const { return m_options.is_write; }
    void enable_irq() const;

private:
    UARTResource(char* buf, size_t size, Options options);
    friend KOwner<UARTResource>;

    void mark_as_finished() { m_capacity = m_size; }
    void fill_from_uart();

    char* m_buf;
    size_t m_size;
    size_t m_capacity;
    Options m_options;
};

class UARTManager;
UARTManager& uart_manager();

class UARTManager {
public:
    void reset();

    void enable_read_irq();
    void enable_write_irq();
    void disable_read_irq();
    void disable_write_irq();

    void set_read_irq(size_t);
    void set_write_irq(size_t);
    void clear_read_irq();
    void clear_write_irq();

    using DidStopOnBreak = bool;
    Pair<size_t, DidStopOnBreak> try_read(char*, size_t bufsize);
    size_t try_write(const char*, size_t bufsize);

private:
    // Only uart_manager can construct
    UARTManager() = default;
    friend UARTManager& uart_manager();

    // These requires special care to disable/enable interrupts, so we make them
    // private and let friends who we trust use these
    char poll_get();
    void poll_put(char ch);
    void poll_write(const char*);
    void poll_write(const char*, size_t);

    friend void console_readline(char*, size_t);
    friend void console(const char*);
    friend void console(const char*, size_t);
    friend void consoleln(const char*);
    friend void consolef(const char*, ...);
    friend class UARTResource;

    template <int offset1, int offset2>
    class InterruptMask {
    public:
        // Compute mask at compile time; this also helps when offset1 == offset2
        constexpr static u32 mask()
        {
            return (1 << offset1) | (1 << offset2);
        }
        static void enable(UARTManager& uart)
        {
            uart.imsc &= ~mask();
        };
        static void disable(UARTManager& uart)
        {
            uart.imsc |= mask();
        };

        InterruptMask(UARTManager& uart) __attribute__((always_inline))
        // Store prev so when the mask is already set (perhaps by a ancestor
        // InterruptMask class in the call stack), we can put it in the
        // previous state we found it in.
        : m_uart_manager(uart)
        , m_prev(m_uart_manager.imsc)
        {
            m_uart_manager.imsc = m_prev & ~mask();
        }

        ~InterruptMask() __attribute__((always_inline))
        {
            m_uart_manager.imsc = m_prev;
        }

    private:
        UARTManager& m_uart_manager;
        const u32 m_prev;
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

void uart_init();

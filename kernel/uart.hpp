#pragma once
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "file.hpp"
#include "wait.hpp"

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

class UARTRequest : public Waitable {
public:
    ~UARTRequest() override = default;
    bool is_finished() const override { return m_size == m_capacity; };

private:
    UARTRequest() = default;
    UARTRequest(char* buf, size_t size, bool is_write_request);

    friend class UARTFile;
    friend void irq_handler();
    friend UARTRequest& uart_request();

    void handle_irq(InterruptsDisabledTag disabled_tag);
    void fill_from_uart();
    void enable_irq();
    size_t size_read_or_written() const { return m_size; };

    char* m_buf = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;
    bool m_is_write_request = false;
};

UARTRequest& uart_request();

class UARTFile : public File {
public:
    ~UARTFile() override = default;
    size_t read(char* buf, size_t at_most_bytes) override;
    size_t write(char* buf, size_t bytes) override;

private:
};

class UARTRegisters;
UARTRegisters& uart_registers();

class UARTRegisters {
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
    // Only uart_registers can construct
    UARTRegisters() = default;
    friend UARTRegisters& uart_registers();

    // These requires special care to disable/enable interrupts, so we make them
    // private and let friends who we trust use these
    char poll_get();
    void poll_put(char ch);
    void poll_write(const char*);
    void poll_write(const char*, size_t);

    friend class UARTPrinter;
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
        static void enable(UARTRegisters& uart)
        {
            uart.imsc &= ~mask();
        };
        static void disable(UARTRegisters& uart)
        {
            uart.imsc |= mask();
        };

        InterruptMask(UARTRegisters& uart) __attribute__((always_inline))
        // Store prev so when the mask is already set (perhaps by a ancestor
        // InterruptMask class in the call stack), we can put it in the
        // previous state we found it in.
        : m_uart_registers(uart)
        , m_prev(m_uart_registers.imsc)
        {
            m_uart_registers.imsc = m_prev & ~mask();
        }

        ~InterruptMask() __attribute__((always_inline))
        {
            m_uart_registers.imsc = m_prev;
        }

    private:
        UARTRegisters& m_uart_registers;
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

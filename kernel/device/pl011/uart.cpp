#include "uart.hpp"
#include "../../device/interrupts.hpp"
#include "../../kmalloc.hpp"
#include "../../arch/panic.hpp"
#include "../../wait.hpp"
#include "../videocore/mailbox.hpp"
#include "../../arch/barrier.hpp"

#include <pine/math.hpp>
#include <pine/bit.hpp>
#include <pine/types.hpp>

void UARTRegisters::poll_write(const char* message)
{
    for (size_t i = 0; message[i] != '\0'; i++)
        poll_put(message[i]);
}

void UARTRegisters::poll_write(const char* message, size_t bytes)
{
    for (size_t i = 0; i < bytes; i++)
        poll_put(message[i]);
}

static inline void spin(u32 count)
{
    asm volatile("__spin_%=: subs %[count], %[count], #1; bne __spin_%=\n"
                 : "=r"(count)
                 : [count] "0"(count)
                 : "cc");
}

consteval Pair<unsigned, unsigned> compute_baud_divisor_register_values()
{
    constexpr unsigned uart_clock_speed_hz = 3000000;
    constexpr unsigned baud_rate = 115200;
    /*
     * The calculation is as follows:
     *
     * https://developer.arm.com/documentation/ddi0183/g/programmers-model/register-descriptions/fractional-baud-rate-register--uartfbrd
     * baud_divisor = uart_clock_speed_hz / (16 * baud_rate)
     *
     * So we get some number. We store the integer part in IBRD (e.g. 1 of 1.67):
     * integer_value = (unsigned) baud_divisor
     *
     * We then store the fractional value in FBRD (e.g. .67 of 1.67). FBRD has
     * a range of 0-63 (6 bits), so we do:
     * fractional_value = (integer_value - baud_divisor) * 63.
     */
    double baud_divisor = static_cast<double>(uart_clock_speed_hz) / (16 * baud_rate);
    unsigned integer_value = static_cast<unsigned>(baud_divisor);
    double fractional_remainder = baud_divisor - static_cast<double>(integer_value);
    unsigned fractional_value = static_cast<unsigned>(fractional_remainder * 63.0);
    return { integer_value, fractional_value };
}

void UARTRegisters::reset()
{
    // Memory barriers required when switching from one peripheral to another
    // See 1.3 in BCM2835 manual
    pine::MemoryBarrier barrier;

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

    // Compute the values at compile time!
    auto [integer_value, fractional_value] = compute_baud_divisor_register_values();

    ibrd = integer_value;
    fbrd = fractional_value;

    // 4: enable FIFO
    // 5/6: 0b11 - 8-bit words
    // rest should be 0 to disable parity, 2-stop bit, etc
    lcrh = (1 << UART_LCRH_FEN) | (1 << UART_LCRH_LWEN0) | (1 << UART_LCRH_LWEN1);

    /*
     * Reset UART and set interrupt masks for all.
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

void UARTRegisters::poll_put(char ch)
{
    // 5: TXFF bit; set when transmit FIFO is full
    while (fr & (1 << UART_FR_TXFF)) {
    }
    dr = ch;
}

char UARTRegisters::poll_get()
{
    // 4: RXFE bit; set when recieve FIFO is empty
    while (fr & (1 << UART_FR_RXFE)) {
    }
    return static_cast<char>(dr);
}

void UARTRegisters::clear_read_irq()
{
    icr &= ~(1u << UART_ICR_RXIC);
}

void UARTRegisters::clear_write_irq()
{
    icr &= ~(1u << UART_ICR_TXIC);
}

void UARTRegisters::enable_read_irq()
{
    ReadInterruptMask::disable(*this);
}

void UARTRegisters::enable_write_irq()
{
    WriteInterruptMask::disable(*this);
}

void UARTRegisters::disable_read_irq()
{
    ReadInterruptMask::enable(*this);
}

void UARTRegisters::disable_write_irq()
{
    WriteInterruptMask::enable(*this);
}

void UARTRegisters::set_read_irq(size_t read_size)
{
    // See Section 13.4 IFLS for details; essentially we can select a trigger
    // at 1/8, 1/4, 1/2, and 7/8 full levels mapped to binary
    u32 fifo_select_bits = pine::min(read_size, static_cast<size_t>(8)) >> 1; // read when we have as most as possible
    pine::overwrite_bit_range(ifls, fifo_select_bits, 3, 5);
}

void UARTRegisters::set_write_irq(size_t write_size)
{
    // See Section 13.4 IFLS for details; essentially we can select a trigger
    // at 1/8, 1/4, 1/2, and 7/8 full levels mapped to binary
    u32 fifo_select_bits = pine::min(write_size, static_cast<size_t>(8)) >> 1; // write when we have as most as possible
    pine::overwrite_bit_range(ifls, fifo_select_bits, 0, 2);
}

Pair<size_t, bool> UARTRegisters::try_read(char* buf, size_t bufsize)
{
    // Barrier required between entry/exit points of peripheral; See 1.3 in BCM2835 manual
    pine::MemoryBarrier barrier;

    bool stopped_on_break = false;
    size_t offset = 0;
    while (offset < bufsize && !(fr & (1 << UART_FR_RXFE))) {
        char ch = static_cast<char>(dr);
        if (ch == '\n' || ch == '\r') {
            poll_put('\n'); // local echo
            stopped_on_break = true;
            break;
        }

        poll_put(ch); // local echo
        *buf = ch;
        buf++;
        offset++;
    }
    return { offset, stopped_on_break };
}

size_t UARTRegisters::try_write(const char* buf, size_t bufsize)
{
    // Barrier required between entry/exit points of peripheral; See 1.3 in BCM2835 manual
    pine::MemoryBarrier barrier;

    size_t offset = 0;
    while (offset < bufsize && !(fr & (1 << UART_FR_TXFF))) {
        char ch = buf[offset++];
        dr = ch;
        if (ch == '\n')
            dr = '\r';
    }
    return offset;
}

UARTRegisters& uart_registers()
{
    static auto* g_uart_registers = reinterpret_cast<UARTRegisters*>(UART_BASE);
    return *g_uart_registers;
}

void uart_init()
{
    uart_registers().reset();
    interrupts_enable_uart();
}

UARTRequest& uart_request()
{
    static UARTRequest g_uart_request;
    return g_uart_request;
}

void reschedule_while_waiting_for(const Waitable&);

ssize_t UARTFile::read(char *buf, size_t at_most_bytes)
{
    PANIC_MESSAGE_IF(!uart_request().is_finished(), "UART request already under operation!");

    auto& request = uart_request();
    request = UARTRequest(buf, at_most_bytes, false);

    // Enable after creation, rather than in constructor, because we use the
    // g_uart_resource handle in our static handle_irq() function; this is not
    // set until after construction and enabling may cause an IRQ to be raised
    request.enable_irq();

    if (!request.is_finished())
        reschedule_while_waiting_for(request);

    PANIC_IF(!request.is_finished());
    return static_cast<ssize_t>(request.size_read_or_written());
}

ssize_t UARTFile::write(char *buf, size_t size)
{
    PANIC_MESSAGE_IF(!uart_request().is_finished(), "UART request already under operation!");

    auto& request = uart_request();
    request = UARTRequest(buf, size, true);

    // Enable after creation, rather than in constructor, because we use the
    // g_uart_resource handle in our static handle_irq() function; this is not
    // set until after construction and enabling may cause an IRQ to be raised
    request.enable_irq();

    if (!request.is_finished())
        reschedule_while_waiting_for(request);

    PANIC_IF(!request.is_finished());
    return static_cast<ssize_t>(request.size_read_or_written());
}

void UARTRequest::enable_irq()
{
    auto& uart = uart_registers();
    // It is possible at this point for data to be ready; by setting the IRQ,
    // we may end up handling that in an IRQ before this call ends
    if (m_is_write_request)
        uart.enable_write_irq();
    else
        uart.enable_read_irq();
}

UARTRequest::UARTRequest(char *buf, size_t size, bool is_write_request)
    : m_buf(buf)
    , m_size(0)
    , m_capacity(size)
    , m_is_write_request(is_write_request)
{
    auto& uart = uart_registers();
    if (m_is_write_request)
        uart.set_write_irq(m_capacity - m_size);
    else
        uart.set_read_irq(m_capacity - m_size);
}

void UARTRequest::fill_from_uart()
{
    auto& uart = uart_registers();
    size_t amount_originally_left = m_capacity - m_size;

    if (m_is_write_request) {
        m_size += uart.try_write(m_buf + m_size, amount_originally_left);
    } else {
        auto amount_and_did_stop = uart.try_read(m_buf + m_size, amount_originally_left);
        m_size += amount_and_did_stop.first;
        if (amount_and_did_stop.second)
            m_capacity = m_size;
    }
}

void UARTRequest::handle_irq(InterruptsDisabledTag)
{
    // We assume interrupts are disabled here, because we don't want nesting
    // of reads/writes to occur.

    auto& uart = uart_registers();
    if (m_is_write_request)
        uart.clear_write_irq();
    else
        uart.clear_read_irq();

    fill_from_uart();

    if (m_size == m_capacity) {
        // Disable it now, instead of in destructor, because we don't want any
        // more IRQs (after returning from this IRQ) being raised that simply
        // return when we handle it
        if (m_is_write_request)
            uart.disable_write_irq();
        else
            uart.disable_read_irq();

        return;
    }

    if (m_is_write_request)
        uart.set_write_irq(m_capacity - m_size);
    else
        uart.set_read_irq(m_capacity - m_size);
}
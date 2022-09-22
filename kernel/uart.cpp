#include "uart.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/math.hpp>
#include <pine/barrier.hpp>
#include <pine/twomath.hpp>

#include <new>

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
    icr &= ~(1 << UART_ICR_RXIC);
}

void UARTRegisters::clear_write_irq()
{
    icr &= ~(1 << UART_ICR_TXIC);
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
    u32 fifo_select_bits = pine::min(read_size, 8u) >> 1; // read when we have as most as possible
    pine::overwrite_bit_range(ifls, fifo_select_bits, 3, 5);
}

void UARTRegisters::set_write_irq(size_t write_size)
{
    // See Section 13.4 IFLS for details; essentially we can select a trigger
    // at 1/8, 1/4, 1/2, and 7/8 full levels mapped to binary
    u32 fifo_select_bits = pine::min(write_size, 8u) >> 1; // write when we have as most as possible
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
    interrupt_registers().enable_uart();
}

static UARTResource* g_uart_resource = nullptr;

UARTResource::UARTResource(char* buf, size_t size, Options options)
    : m_buf(buf)
    , m_size(0)
    , m_capacity(size)
    , m_options(options)
{
    auto& uart = uart_registers();
    if (is_write_request())
        uart.set_write_irq(amount_left());
    else
        uart.set_read_irq(amount_left());
}

UARTResource::~UARTResource()
{
    PANIC_MESSAGE_IF(this != g_uart_resource, "UART Resource completed is not the same as given!")
    g_uart_resource = nullptr;
}

void UARTResource::enable_irq() const
{
    auto& uart = uart_registers();
    // It is possible at this point for data to be ready; by setting the IRQ,
    // we may end up handling that in an IRQ before this call ends
    if (is_write_request())
        uart.enable_write_irq();
    else
        uart.enable_read_irq();
}

void UARTResource::fill_from_uart()
{
    auto& uart = uart_registers();
    size_t amount_originally_left = m_capacity - m_size;

    if (is_write_request()) {
        m_size += uart.try_write(m_buf + m_size, amount_originally_left);
    } else {
        auto amount_and_did_stop = uart.try_read(m_buf + m_size, amount_originally_left);
        m_size += amount_and_did_stop.first;
        if (amount_and_did_stop.second)
            g_uart_resource->mark_as_finished();
    }
}

void UARTResource::handle_irq(InterruptsDisabledTag)
{
    // We assume interrupts are disabled here, because we don't want nesting
    // of reads/writes to occur.

    PANIC_MESSAGE_IF(!g_uart_resource, "Tried to handle IRQ for UART when there is no current request!");
    PANIC_MESSAGE_IF(g_uart_resource->is_finished(), "Tried to handle IRQ after UART resource finished!");

    auto& uart = uart_registers();
    if (g_uart_resource->is_write_request())
        uart.clear_write_irq();
    else
        uart.clear_read_irq();

    g_uart_resource->fill_from_uart();

    if (g_uart_resource->is_finished()) {
        // Disable it now, instead of in destructor, because we don't want any
        // more IRQs (after returning from this IRQ) being raised that simply
        // return when we handle it
        if (g_uart_resource->is_write_request())
            uart.disable_write_irq();
        else
            uart.disable_read_irq();

        return;
    }

    if (g_uart_resource->is_write_request())
        uart.set_write_irq(g_uart_resource->amount_left());
    else
        uart.set_read_irq(g_uart_resource->amount_left());
}

pine::Maybe<KOwner<UARTResource>> UARTResource::try_request(char* buf, size_t bufsize, Options options)
{
    if (g_uart_resource) // Can only handle one request at a time
        panic("UART resource has already been acquired!");

    auto maybe_resource = KOwner<UARTResource>::try_create(kernel_allocator(), buf, bufsize, options);
    if (!maybe_resource) // FIXME: Indicate out of memory!
        return {};

    g_uart_resource = maybe_resource.value().get();

    // Enable after creation, rather than in constructor, because we use the
    // g_uart_resource handle in our static handle_irq() function; this is not
    // set until after construction and enabling may cause an IRQ to be raised
    maybe_resource->enable_irq();
    return maybe_resource;
}

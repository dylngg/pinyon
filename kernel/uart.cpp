#include "uart.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/badmath.hpp>
#include <pine/barrier.hpp>
#include <pine/twomath.hpp>

#include <new>

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

void UARTManager::clear_read_irq()
{
    icr &= ~(1 << UART_ICR_RXIC);
}

void UARTManager::enable_read_irq()
{
    UARTManager::ReadInterruptMask::disable();
}

void UARTManager::disable_read_irq()
{
    UARTManager::ReadInterruptMask::enable();
}

void UARTManager::set_read_irq(size_t read_size)
{
    // See Section 13.4 IFLS for details; essentially we can select a trigger
    // at 1/8, 1/4, 1/2, and 7/8 levels mapped to binary
    u32 fifo_select_bits = min(read_size, 8u) >> 1;
    overwrite_bit_range(ifls, fifo_select_bits, 3, 5);
}

Pair<size_t, bool> UARTManager::try_read(char* buf, size_t bufsize)
{
    MemoryBarrier barrier;

    bool stopped_on_break = false;
    size_t offset = 0;
    while (offset < bufsize && !(fr & (1 << UART_FR_RXFE))) {
        char ch = dr;
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

static UARTResource* g_uart_resource = nullptr;

UARTResource::UARTResource(char* buf, size_t size)
    : m_buf(buf)
    , m_size(0)
    , m_capacity(size)
    , m_is_write(true)
{
    auto& uart = uart_manager();
    uart.set_read_irq(m_size);
    // It is possible at this point for data to be ready; by setting the read
    // IRQ, we may end up handling that in an IRQ before this call ends
    uart.enable_read_irq();
}

UARTResource::~UARTResource()
{
    PANIC_IF(this != g_uart_resource, "UART Resource completed is not the same as given!")
    g_uart_resource = nullptr;
}

void UARTResource::fill_from_uart()
{
    auto& uart = uart_manager();
    size_t amount_originally_left = m_capacity - m_size;

    auto amount_read_and_did_stop = uart.try_read(m_buf + m_size, amount_originally_left);
    size_t amount_read = amount_read_and_did_stop.first;
    m_size += amount_read;

    if (amount_read_and_did_stop.second)
        g_uart_resource->mark_as_finished();
}

void UARTResource::handle_irq()
{
    PANIC_IF(!g_uart_resource, "Tried to handle IRQ for UART when there is no current request!");
    PANIC_IF(g_uart_resource->is_finished(), "Tried to handle IRQ after UART resource finished!");

    auto& uart = uart_manager();
    uart.clear_read_irq();

    g_uart_resource->fill_from_uart();

    if (g_uart_resource->is_finished()) {
        // Disable it now, instead of in destructor, because we don't want any
        // more IRQs being raised that simply return when we handle it
        uart.disable_read_irq();
        return;
    }

    uart.set_read_irq(g_uart_resource->amount_left());
}

Maybe<KOwner<UARTResource>> UARTResource::try_request_read(char* buf, size_t bufsize)
{
    if (g_uart_resource) // Can only handle one request at a time
        return {};

    auto maybe_resource = KOwner<UARTResource>::try_create(buf, bufsize);
    if (!maybe_resource)  // FIXME: Indicate out of memory!
        return {};

    g_uart_resource = maybe_resource.value().get();
    return maybe_resource;
}

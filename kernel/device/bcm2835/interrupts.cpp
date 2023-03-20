#include "interrupts.hpp"
#include "timer.hpp"
#include "../../interrupt_disabler.hpp"
#include "../../device/pl011/uart.hpp"
#include "../../tasks.hpp"
#include "../../barrier.hpp"

void InterruptRegisters::enable_timer() volatile
{
    pine::MemoryBarrier barrier {};
    enable_irq1 = (1 << 1);
}

void InterruptRegisters::enable_uart() volatile
{
    pine::MemoryBarrier barrier {};
    enable_irq2 = (1 << 25);
}

bool InterruptRegisters::timer_pending() const
{
    return pending_irq1 & (1 << 1);
}

bool InterruptRegisters::uart_pending() const
{
    return pending_basic_irq & (1 << 19);
}

InterruptRegisters& interrupt_registers()
{
    static auto* g_interrupt_registers = reinterpret_cast<InterruptRegisters*>(IRQ_BASE);
    return *g_interrupt_registers;
}

extern "C" void enable_irq();

void interrupts_init()
{
    enable_irq();
}

void interrupts_enable_uart()
{
    interrupt_registers().enable_uart();
}

void interrupts_enable_timer()
{
    interrupt_registers().enable_timer();
}

void interrupts_handle_irq(InterruptsDisabledTag disabled_tag)
{
    bool should_reschedule = false;
    auto& irq = interrupt_registers();

    // FIXME: Read pending_basic_irq1 once, then make decision based on that,
    //        rather than reading a bunch of registers here! IRQ handlers
    //        should be quick.
    if (irq.timer_pending()) {
        system_timer().handle_irq(disabled_tag);
        should_reschedule = true;
    }
    if (irq.uart_pending())
        uart_request().handle_irq(disabled_tag);

    if (should_reschedule)
        task_manager().schedule(disabled_tag);
}
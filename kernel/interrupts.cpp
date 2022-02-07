#include "interrupts.hpp"
#include "console.hpp"
#include "panic.hpp"
#include "tasks.hpp"
#include "timer.hpp"
#include "uart.hpp"

#include <pine/barrier.hpp>

extern "C" {

void reset_handler(void)
{
    panic("interrupt:\tResetting. Goodbye.\n");
}

void undefined_instruction_handler(void)
{
    panic("interrupt:\t\033[31mUndefined instruction! halting.\033[0m\n");
}

u32 software_interrupt_handler(Syscall call, u32 arg1, u32 arg2)
{
    // Interrupts are disabled in the SWI handler, and enabled on exit
    auto disabled_tag = InterruptsDisabledTag::promise();
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (call) {
    case Syscall::Yield:
        task_mgr.schedule(disabled_tag);
        break;

    case Syscall::Sleep:
        task.sleep(arg1);
        break;

    case Syscall::Read:
        return task.read((char*)arg1, arg2);

    case Syscall::Write:
        task.write((char*)arg1, arg2);
        break;

    case Syscall::HeapAllocate:
        return task.heap_allocate();

    case Syscall::HeapIncr:
        return task.heap_increase((size_t)arg1);

    case Syscall::Uptime:
        return jiffies();

    case Syscall::CPUTime:
        return task.cputime();

    default:
        consolef("kernel:\tUnknown syscall number %lu\n", (u32) call);
    }

    return 0;
}

void prefetch_abort_handler(void)
{
    panic("interrupt:\t\033[31mPrefetch abort! halting.\033[0m\n");
}

void data_abort_handler(void)
{
    panic("interrupt:\t\033[31mData abort! halting.\033[0m\n");
}

void fast_irq_handler(void)
{
    panic("interrupt:\tHandling fast IRQ...");
}

void irq_handler(void)
{
    auto& irq = irq_manager();

    // FIXME: Read pending_basic_irq1 once, then make decision based on that,
    //        rather than reading a bunch of registers here! IRQ handlers
    //        should be quick.
    if (irq.timer_pending()) {
        {
            InterruptDisabler disabler {};
            system_timer().handle_irq(disabler);
            task_manager().schedule(disabler);
        }
    }
    if (irq.uart_pending()) {
        UARTResource::handle_irq();
    }
}
}

void IRQManager::enable_timer() volatile
{
    MemoryBarrier barrier {};
    enable_irq1 = (1 << 1);
}

void IRQManager::enable_uart() volatile
{
    MemoryBarrier barrier {};
    enable_irq2 = (1 << 25);
}

bool IRQManager::timer_pending() const
{
    return pending_irq1 & (1 << 1);
}

bool IRQManager::uart_pending() const
{
    return pending_basic_irq & (1 << 19);
}

IRQManager& irq_manager()
{
    static auto* g_irq_manager = reinterpret_cast<IRQManager*>(IRQ_BASE);
    return *g_irq_manager;
}

void interrupts_init()
{
    enable_irq();
}

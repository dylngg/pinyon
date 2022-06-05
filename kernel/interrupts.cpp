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

void undefined_instruction_handler(u32 old_cpsr, PtrData old_pc, PtrData old_lr)
{
    panicf("interrupt:\t\033[31mUndefined instruction! halting.\033[0m\n\n"
           "old cpsr: %lu\told pc: %p\told lr: %p\n", old_cpsr, reinterpret_cast<char*>(old_pc),
           reinterpret_cast<char*>(old_lr));
}

u32 software_interrupt_handler(Syscall call, u32 arg1, u32 arg2)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (call) {
    case Syscall::Yield: {
        InterruptDisabler disabler;
        task_mgr.schedule(disabler);
        break;
    }

    case Syscall::Sleep:
        task.sleep(arg1);
        break;

    case Syscall::Read:
        return task.read(reinterpret_cast<char*>(arg1), static_cast<size_t>(arg2));

    case Syscall::Write:
        task.write(reinterpret_cast<char*>(arg1), static_cast<size_t>(arg2));
        break;

    case Syscall::Sbrk:
        return reinterpret_cast<u32>(task.sbrk(static_cast<size_t>(arg1)));

    case Syscall::Uptime:
        return jiffies();

    case Syscall::CPUTime:
        return task.cputime();

    default:
        consolef("kernel:\tUnknown syscall number %lu\n", (u32)call);
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
    bool should_reschedule = false;

    // Interrupts are disabled in the IRQ handler, and enabled on exit; see vector.S
    auto disabled_tag = InterruptsDisabledTag::promise();
    auto& irq = interrupt_registers();

    // FIXME: Read pending_basic_irq1 once, then make decision based on that,
    //        rather than reading a bunch of registers here! IRQ handlers
    //        should be quick.
    if (irq.timer_pending()) {
        system_timer().handle_irq(disabled_tag);
        should_reschedule = true;
    }
    if (irq.uart_pending())
        UARTResource::handle_irq(disabled_tag);

    if (should_reschedule)
        task_manager().schedule(disabled_tag);
}
}

void InterruptRegisters::enable_timer() volatile
{
    MemoryBarrier barrier {};
    enable_irq1 = (1 << 1);
}

void InterruptRegisters::enable_uart() volatile
{
    MemoryBarrier barrier {};
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

void interrupts_init()
{
    enable_irq();
}

#include "interrupts.hpp"
#include "console.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" {

void reset_handler(void)
{
    consolef("interrupt:\tResetting. Goodbye.\n");
    asm volatile("b halt");
}

void undefined_instruction_handler(void)
{
    consolef("interrupt:\t\033[31mUndefined instruction! halting.\033[0m\n");
    asm volatile("b halt");
}

void software_interrupt_handler(void)
{
    TaskManager::manager().schedule();
}

void prefetch_abort_handler(void)
{
    consolef("interrupt:\t\033[31mPrefetch abort! halting.\033[0m\n");
    asm volatile("b halt");
}

void data_abort_handler(void)
{
    consolef("interrupt:\t\033[31mData abort! halting.\033[0m\n");
    asm volatile("b halt");
}

void fast_irq_handler(void)
{
    consolef("interrupt:\tHandling fast IRQ...");
    asm volatile("b halt");
}

void irq_handler(void)
{
    auto timer = SystemTimer::timer();
    if (timer->matched()) {
        increase_jiffies();
        timer->reinit();
        TaskManager::manager().schedule();
    }
}
}

void IRQManager::enable_timer() volatile
{
    MemoryBarrier barrier {};
    enable_irq1 = 0x00000002;
}

void interrupts_init()
{
    enable_irq();
}

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

void software_interrupt_handler(u32 syscall_id, u32 arg)
{
    auto& task_manager = TaskManager::manager();
    auto& task = task_manager.running_task();

    //consolef("Handling syscall %ld with args %ld\n", syscall_id, arg);

    switch (syscall_id) {
    case 0:
        // yield()
        task_manager.schedule();
        break;
    case 1:
        // sleep()
        task.sleep(arg);
        task_manager.schedule();
        break;
    default:
        consolef("kernel:\tUnknown syscall_id number %ld\n", syscall_id);
    }
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

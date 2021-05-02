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

void software_interrupt_handler(u32 syscall_id, u32 arg1, u32 arg2)
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
        task.sleep(arg1);
        task_manager.schedule();
        break;
    case 2:
        // readline()
        task.readline((char*)arg1, arg2);
        break;
    case 3:
        // write()
        task.write((char*)arg1, arg2);
        break;
    case 4: {
        // heap_allocate()
        void** start_addr_ptr = (void**)arg1;
        *start_addr_ptr = (void*)task.heap_allocate();
        break;
    }
    case 5: {
        // heap_incr()
        size_t incr_bytes = (size_t)arg1;
        size_t* actual_incr_size = (size_t*)arg2;
        *actual_incr_size = task.heap_increase(incr_bytes);
        break;
    }
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
        timer->reinit();
        increase_jiffies();

        auto& task_manager = TaskManager::manager();
        task_manager.schedule();
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

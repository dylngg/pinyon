#include "interrupts.hpp"
#include "panic.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" {

void reset_handler(void)
{
    panic("interrupt:\tResetting. Goodbye.\n");
}

void undefined_instruction_handler(void)
{
    panic("interrupt:\t\033[31mUndefined instruction! halting.\033[0m\n");
}

void software_interrupt_handler(u32 syscall_id, u32 arg1, u32 arg2)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();

    //consolef("Handling syscall %ld with args %ld\n", syscall_id, arg);

    switch (syscall_id) {
    case 0:
        // yield()
        task_mgr.schedule();
        break;
    case 1:
        // sleep()
        task.sleep(arg1);
        task_mgr.schedule();
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
    case 6: {
        // uptime()
        u32* jiffies_ret = (u32*)arg1;
        *jiffies_ret = jiffies();
        break;
    }
    case 7: {
        u32* jiffies_ret = (u32*)arg1;
        *jiffies_ret = task.cputime();
        break;
    }
    default:
        consolef("kernel:\tUnknown syscall_id number %ld\n", syscall_id);
    }
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
    auto timer = system_timer();
    if (timer->matched()) {
        u32 jif_diff = timer->jiffies_since_last_match();
        timer->reinit();
        increase_jiffies(jif_diff);
        auto& task_mgr = task_manager();
        task_mgr.schedule();
    }
}
}

void IRQManager::enable_timer() volatile
{
    MemoryBarrier barrier {};
    enable_irq1 = 0x00000002;
}

static auto* g_irq_manager = (IRQManager*)IRQ_BASE;

IRQManager* irq_manager()
{
    return g_irq_manager;
}

void interrupts_init()
{
    enable_irq();
}

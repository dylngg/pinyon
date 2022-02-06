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

u32 software_interrupt_handler(u32 syscall_id, u32 arg1, u32 arg2)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();
    u32 return_value = 0;

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (syscall_id) {
    case 0:
        // yield()
        task_mgr.schedule();
        break;
    case 1:
        // sleep()
        task.sleep(arg1);
        break;
    case 2:
        // read()
        return_value = task.read((char*)arg1, arg2);
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
        consolef("kernel:\tUnknown syscall_id number %lu\n", syscall_id);
    }

    return return_value;
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
        system_timer().handle_irq();
        task_manager().schedule();
    }
    if (irq.uart_pending()) {
        uart_resource().handle_irq();
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

#include "exception.hpp"
#include "../../arch/interrupts.hpp"
#include "../../interrupt_disabler.hpp"
#include "../../arch/panic.hpp"
#include "../../tasks.hpp"
#include "../../timer.hpp"
#include "../../arch/barrier.hpp"
#include "mmu.hpp"
#include "processor.hpp"

#include <pine/bit.hpp>

extern "C" {

void reset_handler(void)
{
    panic("Resetting. Goodbye.");
}

void undefined_instruction_handler(PtrData old_cpsr_as_u32, PtrData old_pc, PtrData old_lr)
{
    auto old_cpsr = CPSR::from_data(old_cpsr_as_u32);
    panic("\033[31mUndefined instruction! halting.\033[0m\n\n"
          "old cpsr:", old_cpsr, "\told pc:", reinterpret_cast<void*>(old_pc), "\told lr: ", reinterpret_cast<void*>(old_lr));
}

u32 software_interrupt_handler(Syscall call, u32 arg1, u32 arg2, u32 arg3)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (call) {
    case Syscall::Exit: {
        InterruptDisabler disabler;
        int code = static_cast<int>(arg1);
        task_mgr.exit_running_task(disabler, code);
        break;
    }

    case Syscall::Yield: {
        InterruptDisabler disabler;
        task_mgr.schedule(disabler);
        break;
    }

    case Syscall::Sleep:
        task.sleep(arg1);
        break;

    case Syscall::Open:
        return pine::bit_cast<u32>(task.open(reinterpret_cast<const char*>(arg1), pine::bit_cast<FileMode>(arg2)));

    case Syscall::Read:
        return pine::bit_cast<ssize_t>(task.read(pine::bit_cast<int>(arg1), reinterpret_cast<char*>(arg2), pine::bit_cast<size_t>(arg3)));

    case Syscall::Write:
        return pine::bit_cast<ssize_t>(task.write(pine::bit_cast<int>(arg1), reinterpret_cast<char*>(arg2), pine::bit_cast<size_t>(arg3)));

    case Syscall::Close:
        return pine::bit_cast<ssize_t>(task.close(pine::bit_cast<int>(arg1)));

    case Syscall::Dup:
        return pine::bit_cast<u32>(task.dup(pine::bit_cast<int>(arg1)));

    case Syscall::Sbrk:
        return reinterpret_cast<u32>(task.sbrk(pine::bit_cast<size_t>(arg1)));

    case Syscall::Uptime:
        return jiffies();

    case Syscall::CPUTime:
        return task.cputime();

    default:
        consoleln("kernel:\tUnknown syscall number ", (u32)call);
    }

    return 0;
}

void prefetch_abort_handler(void)
{
    panic("interrupt:\t\033[31mPrefetch abort! halting.\033[0m");
}

void data_abort_handler(PtrData old_cpsr_as_u32, PtrData old_pc, PtrData addr)
{
    auto old_cpsr = CPSR::from_data(old_cpsr_as_u32);
    panic("interrupt:\t\033[31mData abort! halting.\033[0m\n\n"
          "old cpsr:", old_cpsr, "\told pc:", reinterpret_cast<void*>(old_pc), "\taddr:", reinterpret_cast<void*>(addr), "\n",
          mmu::l1_table());
}

void fast_irq_handler(void)
{
    panic("interrupt:\tHandling fast IRQ...");
}

void irq_handler(void)
{
    // Interrupts are disabled in the IRQ handler, and enabled on exit; see vector.S
    interrupts_handle_irq(InterruptsDisabledTag::promise());
}
}

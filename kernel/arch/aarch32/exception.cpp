#include "exception.hpp"
#include "../../device/interrupts.hpp"
#include "../../syscall.hpp"
#include "../../arch/panic.hpp"
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

// FIXME: We should not be assuming that call is a valid enumeration of Syscall!
PtrData software_interrupt_handler(Syscall call, PtrData arg1, PtrData arg2, PtrData arg3)
{
    return handle_syscall(call, arg1, arg2, arg3);
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

#pragma once
#include <pine/types.hpp>
#include <pine/syscall.hpp>

// Normally we'd have one the following GCC attributes on our handlers to
// signify that GCC should save registers used in the handler on start and
// return, but this is not exactly what we want for the disabled ones, since we
// do our own register saving in vector.S.
// __attribute__((interrupt("IRQ")));
// __attribute__((interrupt("SWI")));
//  __attribute__((interrupt("UNDEF")));

extern "C" {
void reset_handler(void) __attribute__((interrupt("ABORT")));

void undefined_instruction_handler(PtrData old_cpsr, PtrData old_pc, PtrData old_lr);

PtrData software_interrupt_handler(PtrData call, PtrData arg1, PtrData arg2, PtrData arg3);

void prefetch_abort_handler(void) __attribute__((interrupt("ABORT")));

void data_abort_handler(PtrData old_cpsr, PtrData old_pc, PtrData addr);

void fast_irq_handler(void) __attribute__((interrupt("FIQ")));

void irq_handler(void);

// See vector.S for implementation
void enable_irq(void);
}

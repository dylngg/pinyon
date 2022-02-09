#pragma once
#include <pine/types.hpp>

/*
 * The Rasbperry Pi has 72 IRQs. It is particularly weird because the GPU
 * shares interrupts with the CPU. Also, documentation in the BCM2835 manual
 * is piss poor but see page 109 for [a lack of] details.
 */
#define IRQ_BASE 0x3F00B200

struct IRQManager {
    void enable_timer() volatile;
    void enable_uart() volatile;
    bool timer_pending() const;
    bool uart_pending() const;

private:
    /*
     * See section 7.5 on page 112 in the BCM2835 manual for the
     * definition of these
     */
    volatile u32 pending_basic_irq;
    volatile u32 pending_irq1;
    volatile u32 pending_irq2;
    volatile u32 fiq_ctrl;
    volatile u32 enable_irq1;
    volatile u32 enable_irq2;
    volatile u32 enable_basic_irq;
    volatile u32 disable_irq1;
    volatile u32 disable_irq2;
    volatile u32 disable_basic_irq;
};

struct InterruptDisabler {
    InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("cpsid i");
    }

    ~InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("cpsie i");
    }
};

struct InterruptsDisabledTag {
    InterruptsDisabledTag(const InterruptDisabler&) {};
    static InterruptsDisabledTag promise() { return InterruptsDisabledTag {}; };

private:
    InterruptsDisabledTag() = default;
};

enum class Syscall : u32 {
    Yield = 0,
    Sleep,
    Read,
    Write,
    HeapAllocate,
    HeapIncr,
    Uptime,
    CPUTime,
};

IRQManager& irq_manager();

void interrupts_init();

// Normally we'd have one the following GCC attributes on our handlers to
// signify that GCC should save registers used in the handler on start and
// return, but this is not exactly what we want for the disabled ones, since we
// do our own register saving in vector.S.
// __attribute__((interrupt("IRQ")));
// __attribute__((interrupt("SWI")));
//  __attribute__((interrupt("UNDEF")));

extern "C" {
void reset_handler(void) __attribute__((interrupt("ABORT")));

void undefined_instruction_handler(u32 old_cpsr, u32 old_pc, u32 old_lr);

u32 software_interrupt_handler(Syscall call, u32 arg1, u32 arg2);

void prefetch_abort_handler(void) __attribute__((interrupt("ABORT")));

void data_abort_handler(void) __attribute__((interrupt("ABORT")));

void fast_irq_handler(void) __attribute__((interrupt("FIQ")));

void irq_handler(void);

// See vector.S for implementation
void enable_irq(void);
}

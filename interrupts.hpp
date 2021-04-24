#pragma once
#include "console.hpp"
#include <pine/types.hpp>

/*
 * The Rasbperry Pi has 72 IRQs. It is particularly weird because the GPU
 * shares interrupts with the CPU. Also, documentation in the BCM2835 manual
 * is piss poor but see page 109 for [a lack of] details.
 */
#define IRQ_BASE 0x3F00B200

struct IRQManager;

static volatile IRQManager* g_irq_manager = (IRQManager*)IRQ_BASE;

struct IRQManager {
    IRQManager() {};

    void enable_timer() volatile;
    static volatile IRQManager* manager()
    {
        return g_irq_manager;
    }

private:
    /*
     * See section 7.5 on page 112 in the BCM2835 manual for the
     * definition of these
     */
    u32 pending_basic_irq = 0;
    u32 pending_irq1 = 0;
    u32 pending_irq2 = 0;
    u32 fiq_ctrl = 0;
    u32 enable_irq1 = 0;
    u32 enable_irq2 = 0;
    u32 enable_basic_irq = 0;
    u32 disable_irq1 = 0;
    u32 disable_irq2 = 0;
    u32 disable_basic_irq = 0;
};

extern "C" {

void reset_handler(void) __attribute__((interrupt("ABORT")));

void undefined_instruction_handler(void) __attribute__((interrupt("UNDEF")));

void software_interrupt_handler(void) __attribute__((interrupt("SWI")));

void prefetch_abort_handler(void) __attribute__((interrupt("ABORT")));

void data_abort_handler(void) __attribute__((interrupt("ABORT")));

void fast_irq_handler(void) __attribute__((interrupt("FIQ")));

// Normally we'd have the following GCC attribute to signify that GCC should
// save any registers used in the handler, but I'm not convinced that GCC is
// doing the right thing since I get strange issues when I don't use my custom
// IRQ register saving stuff in vector.S...
// void irq_handler(void) __attribute__((interrupt("IRQ")));
void irq_handler(void);

// See vector.S for implementation
void enable_irq(void);
}

void interrupts_init();

// Forces a data memory barrier on creation and destroy. This is only needed
// when the underlying virtual memory map is not strongly ordered.
class MemoryBarrier {
public:
    MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }

    ~MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }

    static void sync() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }
};
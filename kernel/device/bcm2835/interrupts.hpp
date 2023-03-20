#pragma once
#include <pine/types.hpp>

/*
 * The Rasbperry Pi has 72 IRQs. It is particularly weird because the GPU
 * shares interrupts with the CPU. Also, documentation in the BCM2835 manual
 * is not the greatest but see page 109 for details.
 */
#define IRQ_BASE 0x3F00B200

struct InterruptRegisters {
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

InterruptRegisters& interrupt_registers();

#pragma once
#include <pine/types.hpp>

/*
 * The system timer on the BCM2835 is uh... interesting.
 *
 * It has a single counter that contiously goes up. You can match against that
 * counter using four comparison registers, but the GPU uses registers 0 and 2,
 * so you can't use those (undocumented of course).
 *
 * When your timer goes off your interrupt is supposed to set a new counter
 * value based on the current counter. This of course leads to race conditions.
 */

// The CPU runs at 1 MHz
#define TIMER_HZ 1000000

// Interrupt ourselves every 1s, with QEMU this is the safe option, otherwise
// we might miss a IRQ and are left with no timer.
#define SYS_HZ 1

/*
 * System timer base. See section 12.1 on page 172 in the BCM2835 Manual for
 * details.
 */
#define TIM_BASE 0x3F003000

struct SystemTimer {
public:
    void init() volatile;

    void reinit() volatile;

    bool matched() const volatile;

private:
    volatile u32 control;
    volatile u32 lower_bits;
    volatile u32 upper_bits;
    volatile u32 compare0;
    volatile u32 compare1;
    volatile u32 compare2;
    volatile u32 compare3;
};

volatile SystemTimer* system_timer();

void timer_init();

void increase_jiffies();

u32 jiffies();
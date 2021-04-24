#pragma once
#include <pine/types.hpp>

// The CPU runs at 1 MHz
#define TIMER_HZ 1000000

// Interrupt ourselves every 100ms
#define SYS_HZ 100

/*
 * System timer base. See section 12.1 on page 172 in the BCM2835 Manual for
 * details.
 */
#define TIM_BASE 0x3F003000

struct SystemTimer;

static volatile SystemTimer* g_system_timer = (volatile SystemTimer*)TIM_BASE;

struct SystemTimer {
    void init() volatile;

    void reinit() volatile;

    bool matched() const volatile;
    static volatile SystemTimer* timer()
    {
        return g_system_timer;
    }

private:
    volatile u32 control;
    volatile u32 lower_bits;
    volatile u32 upper_bits;
    volatile u32 compare0;
    volatile u32 compare1;
    volatile u32 compare2;
    volatile u32 compare3;
};

void timer_init();

void increase_jiffies();

u32 jiffies();
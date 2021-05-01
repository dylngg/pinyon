#include "timer.hpp"
#include "interrupts.hpp"
#include <pine/barrier.hpp>

static u32 timer_counter_match;

void SystemTimer::init() volatile
{
    /*
     * Use system timer 1. GPU uses 0 and 2.
     *
     * See page 172 in BCM2835 ARM Peripherals for details
     */
    timer_counter_match = TIMER_HZ / SYS_HZ;
    {
        MemoryBarrier barrier {};
        compare1 = lower_bits + timer_counter_match;

        // Clear timer match flag
        control |= (1 << 1);
    }
}

void SystemTimer::reinit() volatile
{
    MemoryBarrier barrier {};
    /*
     * The timer must be reinitialized upon every interrupt, so we reinit it
     * here. Each initialization should be the current timer value + the time
     * you want to wait.
     */
    compare1 = lower_bits + timer_counter_match;

    // Clear timer match flag
    control |= (1 << 1);
}

bool SystemTimer::matched() const volatile
{
    MemoryBarrier::sync();
    return (control & 0x2) > 0;
}

void timer_init()
{
    SystemTimer::timer()->init();
    IRQManager::manager()->enable_timer();
}

// 32 bits should ought to be enough for anyone ;)
static u32 jiffies_since_boot;

void increase_jiffies()
{
    jiffies_since_boot++;
}

u32 jiffies()
{
    return jiffies_since_boot;
}
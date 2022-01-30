#include "timer.hpp"
#include "interrupts.hpp"
#include "panic.hpp"

#include <pine/barrier.hpp>
#include <pine/units.hpp>

// 32 bits should ought to be enough for anyone ;)
static u32 jiffies_since_boot;
static u32 timer_counter_match;

void SystemTimer::init()
{
    /*
     * Use system timer 1. GPU uses 0 and 2.
     *
     * See page 172 in BCM2835 ARM Peripherals for details
     */
    timer_counter_match = TIMER_HZ >> SYS_HZ_BITS;
    {
        MemoryBarrier barrier {};
        compare1 = lower_bits + timer_counter_match;
        compare3 = lower_bits + (timer_counter_match << FALLBACK_SYS_HZ_SCALER_BITS);

        // Clear timer match flag
        control |= (1 << 1);
    }

    jiffies_since_boot = 0;
}

void SystemTimer::handle_irq()
{
    PANIC_IF(!matched(), "IRQ handler for timer called, but not needed!");

    u32 jif_diff = jiffies_since_last_match();
    reinit();
    jiffies_since_boot += jif_diff;
}

void SystemTimer::reinit()
{
    MemoryBarrier barrier {};
    /*
     * The timer must be reinitialized upon every interrupt, so we reinit it
     * here. Each initialization should be the current timer value + the time
     * you want to wait.
     *
     * There is overhead in re-initing the timer, and this is fixed by only
     * waiting our SYS_HZ period minus the time left: compare1 + counter_match
     *
     * But this does not work well alone when emulating in QEMU. The system
     * timer simulated by QEMU _is_ accurate to real time, but the clock cycles
     * required to simulate the ARM system is significant (and we are dependent
     * on scheduling by the host OS). This in practice can lead to issues if
     * interrupts are disabled for longer than the SYS_HZ (in real time),
     * causing a simple 'lower_bits + timer_counter_match' to be _less_ than
     * compare1, resulting in no IRQs until lower_bits overflows (since it's
     * 32 bits).
     *
     * The effect of this issue is that the current task never gets rescheduled.
     *
     * My hack here is to also set the compare{3} register to a much larger
     * period, hedging that a much larger time period will catch this. Then,
     * we'll check whether that was matched against in our IRQ and take appropriate
     * action (increasing jiffies accordingly).
     *
     * This "fix" of course can lead to inconsistent schedule times, but accounting
     * for that in the scheduler is too complicated for me to deal with at the current
     * moment.
     */

    // Try harder to ensure timer IRQ happens, potentially missing jiffies. From testing,
    // we still need compare3 because there are race conditions with the host scheduler
    // (I'm guessing that's the reason)
    while (compare1 < lower_bits)
        compare1 += timer_counter_match;

    compare3 = lower_bits + (timer_counter_match << FALLBACK_SYS_HZ_SCALER_BITS);

    // Clear timer match flag
    control |= (1 << 1);
}

u32 SystemTimer::jiffies_since_last_match() const
{
    MemoryBarrier::sync();
    if (compare3 < lower_bits) {
        //consoleln("\033[0;31mkernel:\tFallback timer match encountered!\033[0m");
        return FALLBACK_SYS_HZ_SCALER;
    }

    return 1;
}

bool SystemTimer::matched() const
{
    MemoryBarrier::sync();
    return (control & 0x3) > 0;
}

SystemTimer& system_timer()
{
    static auto* g_system_timer = reinterpret_cast<SystemTimer*>(TIM_BASE);
    return *g_system_timer;
}

void timer_init()
{
    system_timer().init();
    irq_manager().enable_timer();
}

u32 jiffies()
{
    return jiffies_since_boot;
}

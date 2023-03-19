#ifdef AARCH64
#elif AARCH32
#include "console.hpp"
#include "device/bcm2835/display.hpp"
#include "interrupts.hpp"
#include "device/videocore/mailbox.hpp"
#include "tasks.hpp"
#include "timer.hpp"
#include "arch/aarch32/mmu.hpp"
#include "device/pl011/uart.hpp"
#else
#error Architecture not defined
#endif

#include <pine/types.hpp>
#include "panic.hpp"

extern "C" {
void init();  // Export init symbol as C
}

/*
 * operator delete is required with virtual destructors.
 */
void operator delete(void* ptr)
{
    panic("operator delete got called! (new() doesn't exist?!): ptr", ptr);
}

void operator delete(void* ptr, size_t size) // C++14 specialization
{
    panic("operator delete got called! (new() doesn't exist?!): ptr", ptr, ", size", size);
}

extern "C" {

void __cxa_pure_virtual()
{
    panic("A pure virtual function was called!");
}

}

void init()
{
#ifdef AARCH32
    interrupts_init();
    uart_init();
    console("Initializing... ");
    console("memory ");
    mmu_init();
    console("timer ");
    timer_init();
    console("display");
    display_init(1024, 756);
    consoleln();

    const char* pinyon = "\033[0;33mPinyon\033[0m";
    const char* pine = "+\033[0;32mPine\033[0m";
    consolef("Welcome to %s%s! (%c) %d\n", pinyon, pine, 'c', 2021);

    auto maybe_serial = try_retrieve_serial_num_from_mailbox();
    PANIC_IF(!maybe_serial);
    auto serial = *maybe_serial;
    consolef("Serial: %#x%x\n", serial.bottom, serial.top);

    consoleln("Use 'help' for a list of commands to run.");

    tasks_init();
#endif
}

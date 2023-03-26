#ifdef AARCH64
#elif AARCH32
#include "tasks.hpp"
#include "arch/aarch32/mmu.hpp"
#else
#error Architecture not defined
#endif

#include "arch/panic.hpp"
#include "console.hpp"
#include "device/bcm2835/display.hpp"
#include "device/interrupts.hpp"
#include "device/pl011/uart.hpp"
#include "device/timer.hpp"
#include "device/videocore/mailbox.hpp"

#include <pine/types.hpp>

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
    interrupts_init();
    uart_init();
    console("Initializing... ");
#ifndef AARCH64
    console("memory ");
    mmu_init();
#endif
    console("timer ");
    timer_init();
    console("display");
    display_init(1024, 756);
    consoleln();

    const char* pinyon = "\033[0;33mPinyon\033[0m";
    const char* pine = "+\033[0;32mPine\033[0m";
    consolef("Welcome to %s%s! (%c) %d-%d\n", pinyon, pine, 'c', 2021, 2023);

    auto maybe_serial = try_retrieve_serial_num_from_mailbox();
    PANIC_IF(!maybe_serial);
    auto serial = *maybe_serial;
    consolef("Serial: %#lx%lx\n", static_cast<unsigned long>(serial.bottom), static_cast<unsigned long>(serial.top));

    consoleln("Use 'help' for a list of commands to run.");

#ifndef AARCH64
    tasks_init();
#endif
}

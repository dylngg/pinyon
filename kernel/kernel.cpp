#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" void init();

/*
 * operator delete is required with virtual destructors.
 */
void operator delete(void* ptr)
{
    panic("operator delete got called! (new() doesn't exist?!): ptr", ptr);
}

void operator delete(void* ptr, size_t size)  // C++14 specialization
{
    panic("operator delete got called! (new() doesn't exist?!): ptr", ptr, ", size", size);
}

void init()
{
    interrupts_init();
    uart_init();
    console("Initializing... ");
    console("memory ");
    console("timer ");
    timer_init();
    consoleln();

    const char* pinyon = "\033[0;33mPinyon\033[0m";
    const char* pine = "+\033[0;32mPine\033[0m";
    consolef("Welcome to %s%s! (%c) %d\n", pinyon, pine, 'c', 2021);
    consoleln("Use 'help' for a list of commands to run.");

    tasks_init();
}

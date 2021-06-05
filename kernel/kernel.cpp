#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" void init();

void init()
{
    uart_init();
    console("Initializing... ");
    console("memory ");
    kmem_init();
    console("timer ");
    timer_init();
    console("interrupts");
    interrupts_init();
    consoleln("");

    // No good reason for this, beyond using new kmalloc calls
    char* pinyon = (char*)kmalloc(19);
    strcopy(pinyon, "\033[0;33mPinyon\033[0m");
    char* pine = (char*)kmalloc(17);
    strcopy(pine, "+\033[0;32mPine\033[0m");

    consolef("Welcome to %s%s! (%c) %d\n", pinyon, pine, 'c', 2021);
    consoleln("Use 'help' for a list of commands to run.");

    kfree((void*)pinyon);
    kfree((void*)pine);

    tasks_init();
}

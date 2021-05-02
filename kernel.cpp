#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" void init();

void init()
{
    console_init();
    console("Initializing... ");
    console("memory ");
    kmem_init();
    console("timer ");
    timer_init();
    console("interrupts");
    interrupts_init();
    consoleln("");

    // No good reason for this, beyond using new kmalloc calls
    char* pinyon = (char*)kmalloc(7);
    strcpy(pinyon, "Pinyon");
    char* pine = (char*)kmalloc(6);
    strcpy(pine, "+Pine");

    consolef("Welcome to %s%s! (%c) %d\n", pinyon, pine, 'c', 2021);

    kfree((void*)pinyon);
    kfree((void*)pine);

    tasks_init();
}

#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "tasks.hpp"
#include "timer.hpp"

extern "C" void init();

void init()
{
    console_init();
    consoleln("Initializing... ");
    timer_init();
    interrupts_init();
    kmalloc_init();

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

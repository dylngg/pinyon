#include "kmalloc.hpp"
#include "console.hpp"

extern "C" [[noreturn]] void init();

void init()
{
    consoleln("Initializing... ");
    kmalloc_init();

    // No good reason for this, beyond using new kmalloc calls
    char* pinyon = (char*)kmalloc(7);
    strcpy(pinyon, "Pinyon");
    char* pine = (char*)kmalloc(6);
    strcpy(pine, "+Pine");

    consolef("Welcome to %s%s! (%c) %d\n", pinyon, pine, 'c', 2021);

    kfree((void*)pinyon);
    kfree((void*)pine);

    while (1) {
    }
}

#include <pine/console.hpp>

extern "C" [[noreturn]] void init();

void init()
{
    console("Initializing... ");
    // Do initializing stuff here...
    consoleln("");

    consolef("Welcome to %s! (%c) %d\n", "Pinyon", 'c', 2021);

    while (1) {
    }
}

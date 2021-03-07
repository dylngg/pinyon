#include <stddef.h>

extern "C" [[noreturn]] void init();

// Magic number I found at https://jsandler18.github.io/explanations/kernel_c.html
// Note: This is specific to the Raspberry Pi 2/3
#define UART0_BASE 0x3F201000

void write_message(const char* message)
{
    for (int i = 0; message[i]; i++) {
        *(volatile size_t*)(UART0_BASE) = (size_t)message[i];
    }
}

void init()
{
    const char* greeting = "Why hello there! :D\n";
    write_message(greeting);

    while (1) {
    }
}

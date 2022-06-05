#include "panic.hpp"

#include "console.hpp"
#include <pine/string.hpp>

#define PANIC_BUFFER_SIZE 512
/*
 * Save some space in case things really go wrong and we cannot malloc
 * Ideally we could could allocate this on bootup, but for now putting it in
 * the image works
 */
static char panic_buffer[PANIC_BUFFER_SIZE];

// Always flatten (inline into caller) since panicf calls panic after some
// setup and we want the LR to be the function that called panicf, not panicf
// itself.
__attribute__((flatten)) __attribute__((always_inline))
static void panic_impl(const char* message)
{
    PtrData sp;
    PtrData lr;
    u32 cpsr;
    asm volatile("mov %0, sp"
                 : "=r"(sp));
    asm volatile("mov %0, lr"
                 : "=r"(lr));
    asm volatile("mrs %0, cpsr"
                 : "=r"(cpsr));
    consoleln("\nKERNEL PANIC! (!!!)");
    if (message[0] != '\0')
        consoleln(message);

    sbufprintf(panic_buffer, PANIC_BUFFER_SIZE, "cpsr: %lu, sp: %p, lr: %p", cpsr,
               reinterpret_cast<char*>(sp), reinterpret_cast<char*>(lr));
    consoleln(panic_buffer);
    asm volatile("b halt");
}

void panic(const char* message)
{
    panic_impl(message);
}

void panicf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsbufprintf(panic_buffer, PANIC_BUFFER_SIZE, fmt, args);
    panic_impl(panic_buffer);
}

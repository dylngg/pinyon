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

void panic(const char* message)
{
    u32 sp;
    u32 lr;
    u32 cpsr;
    asm volatile("mov %0, sp"
                 : "=r"(sp));
    asm volatile("mov %0, lr"
                 : "=r"(lr));
    asm volatile("mrs %0, cpsr"
                 : "=r"(cpsr));
    consoleln("KERNEL PANIC! (!!!)");
    consoleln(panic_buffer);
    sbufprintf(panic_buffer, PANIC_BUFFER_SIZE, "cpsr: %lu, sp: %p, lr: %p", cpsr, (void*)sp, (void*)lr);
    consoleln(panic_buffer);

    if (strlen(message) > 0) {
        console("message: ");
        consoleln(message);
    }
    asm volatile("b halt");
}

void panicf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsbufprintf(panic_buffer, PANIC_BUFFER_SIZE, fmt, args);
    console("message: ");
    consoleln(panic_buffer);
    panic("");
}

void panic_if(bool condition, const char* message)
{
    if (condition)
        panic(message);
}

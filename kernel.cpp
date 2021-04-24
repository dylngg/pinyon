#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "timer.hpp"

extern "C" void init();

static void builtin_memstat()
{
    auto malloc_stats = kmemstats();
    unsigned int pct_mem_util = (malloc_stats.amount_used * 100) / malloc_stats.heap_size;
    consolef("heap size: %d bytes\nused: %d bytes (%u%% util)\nnmallocs: %d\nnfrees: %d\n",
        malloc_stats.heap_size, malloc_stats.amount_used, pct_mem_util, malloc_stats.num_mallocs,
        malloc_stats.num_frees);
}

static void builtin_uptime()
{
    auto jifs = jiffies();
    auto seconds = jifs / SYS_HZ;
    consolef("uptime: %lus (%lu jif)\n", seconds, jifs);
}

static void start_shell()
{
    char* buf = (char*)kmalloc(1024);

    for (;;) {
        console("# ");
        console_readline(buf, 1024);

        if (strcmp(buf, "exit") == 0)
            break;
        if (strcmp(buf, "memstat") == 0) {
            builtin_memstat();
            continue;
        }
        if (strcmp(buf, "uptime") == 0) {
            builtin_uptime();
            continue;
        }
        consolef("Unknown command '%s'\n", buf);
    }

    kfree((void*)buf);
}

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

    start_shell();

    consoleln("goodbye.");
    asm volatile("b halt");
}

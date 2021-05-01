#include "shell.hpp"
#include "kmalloc.hpp"
#include "lib.hpp"
#include "timer.hpp"

static void builtin_memstat()
{
    auto malloc_stats = kmemstats();
    unsigned int pct_mem_util = (malloc_stats.amount_used * 100) / malloc_stats.heap_size;
    printf("heap size: %d bytes\nused: %d bytes (%u%% util)\nnmallocs: %d\nnfrees: %d\n",
        malloc_stats.heap_size, malloc_stats.amount_used, pct_mem_util, malloc_stats.num_mallocs,
        malloc_stats.num_frees);
}

static void builtin_uptime()
{
    auto jifs = jiffies();
    auto seconds = jifs / SYS_HZ;
    printf("uptime: %lus (%lu jif)\n", seconds, jifs);
}

extern "C" {

void shell()
{
    char* buf = (char*)kmalloc(1024);

    for (;;) {
        printf("# ");
        readline(buf, 1024);

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
        if (strcmp(buf, "yield") == 0) {
            yield();
            asm volatile("swi 0"); // yield
            continue;
        }
        if (strcmp(buf, "sleep") == 0) {
            printf("Sleeping for 2 seconds.");
            sleep(2);
            continue;
        }
        printf("Unknown command '%s'\n", buf);
    }

    kfree((void*)buf);

    printf("goodbye.\n");
    asm volatile("b halt");
}
}
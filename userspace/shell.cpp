#include "shell.hpp"
#include "lib.hpp"
#include <pine/units.hpp>

static void builtin_memstat()
{
    auto malloc_stats = memstats();
    unsigned int pct_mem_util = (malloc_stats.amount_used * 100) / malloc_stats.heap_size;
    printf("heap size: %d bytes\nused: %d bytes (%u%% util)\nnmallocs: %d\nnfrees: %d\n",
        malloc_stats.heap_size, malloc_stats.amount_used, pct_mem_util, malloc_stats.num_mallocs,
        malloc_stats.num_frees);
}

static void builtin_uptime()
{
    auto uptime_jiffies = uptime();
    auto uptime_seconds = uptime_jiffies / SYS_HZ;
    auto cputime_jiffies = cputime();
    auto cpu_usage = cputime_jiffies  * 100 / uptime_jiffies;
    printf("up %lds, usage: %lu%% (%lu / %lu jiffies)\n", uptime_seconds, cpu_usage, cputime_jiffies, uptime_jiffies);
}

extern "C" {

void shell()
{
    mem_init();
    char* buf = (char*)malloc(1024);

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
            continue;
        }
        if (strcmp(buf, "sleep") == 0) {
            printf("Sleeping for 2 seconds.\n");
            sleep(2);
            continue;
        }
        printf("Unknown command '%s'\n", buf);
    }

    free((void*)buf);

    printf("goodbye.\n");
    asm volatile("b halt");
}
}
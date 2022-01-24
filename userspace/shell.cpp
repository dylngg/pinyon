#include "shell.hpp"
#include "lib.hpp"
#include <pine/units.hpp>

static void builtin_memstat()
{
    auto malloc_stats = memstats();
    unsigned int pct_of_heap_requested = (malloc_stats.amount_requested * 100) / malloc_stats.heap_size;
    int overhead_bytes = malloc_stats.amount_allocated - malloc_stats.amount_requested;
    unsigned int pct_of_heap_overhead = (overhead_bytes * 100) / malloc_stats.heap_size;
    printf("heap size: %d bytes\n"
           "requested: %u bytes (%u%% of heap)\n"
           "overhead: %d bytes (%u%% of heap)\n"
           "nmallocs: %llu\nnfrees: %llu\n",
        malloc_stats.heap_size,
        malloc_stats.amount_requested, pct_of_heap_requested,
        overhead_bytes, pct_of_heap_overhead,
        malloc_stats.num_mallocs, malloc_stats.num_frees);
}

static void builtin_uptime()
{
    auto uptime_jiffies = uptime();
    auto uptime_seconds = uptime_jiffies >> SYS_HZ_BITS;
    auto cputime_jiffies = cputime();
    auto cpu_usage = cputime_jiffies * 100 / uptime_jiffies;
    printf("up %lds, usage: %lu%% (%lu / %lu jiffies)\n", uptime_seconds, cpu_usage, cputime_jiffies, uptime_jiffies);
}

extern "C" {

void shell()
{
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
        if (strcmp(buf, "help") == 0) {
            printf("The following commands are available to you:\n");
            printf("  - memstat\tProvides statistics on the amount of memory used by this task.\n");
            printf("  - uptime\tProvides statistics on the time since boot in seconds, as well as the CPU time used by this task.\n");
            printf("  - yield\tYields to the spin task. The spin task simply spins until it is preempted. You should see control return to this task shortly.\n");
            printf("  - sleep\tPuts this task to sleep for 2 seconds.\n");
            printf("  - exit\tSays goodbye. Please hit Ctrl-C to actually exit.\n");
            printf("\n");
            printf("Known Bugs (because we're honest around here!):\n");
            printf("  - You may encounter periods of a second or more of unresponsiveness. This is a known issue related to emulation and the design the system timer peripheral.\n");
            printf("  - 'sleep' can sleep for more than 2 seconds. Related to the above issue.\n");
            printf("  - Time slowly drifts away from real time.\n");
            continue;
        }
        printf("Unknown command '%s'. Use 'help'.\n", buf);
    }

    free((void*)buf);

    printf("goodbye.\n");
    asm volatile("b halt");
}
}

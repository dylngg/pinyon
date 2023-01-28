#include "shell.hpp"
#include "lib.hpp"

#include <pine/math.hpp>
#include <pine/units.hpp>

using namespace pine;

static void builtin_memstat()
{
    auto malloc_stats = memstats();
    unsigned int pct_of_heap = (malloc_stats.used_size * 100) / malloc_stats.heap_size;
    printf("heap size: %d bytes\n"
           "requested: %u bytes (%u%% of heap)\n"
           "nmallocs: %lu\nnfrees: %lu\n",
           malloc_stats.heap_size,
           malloc_stats.used_size,
           pct_of_heap,
           malloc_stats.num_mallocs,
           malloc_stats.num_frees);
}

static void builtin_uptime()
{
    auto uptime_jiffies = uptime();
    auto uptime_seconds = uptime_jiffies >> SYS_HZ_BITS;
    auto cputime_jiffies = cputime();
    auto cpu_usage = cputime_jiffies * 100u / pine::max(uptime_jiffies, static_cast<decltype(cputime_jiffies)>(1));
    printf("up %lds, usage: %lu%% (%lu / %lu jiffies)\n", uptime_seconds, cpu_usage, cputime_jiffies, uptime_jiffies);
}

extern "C" {

void shell()
{
    TVector<char> buf(mem_allocator());
    if (!buf.ensure(1024)) {
        printf("Could not allocate memory for buf!!\n");
        exit(1);
    }

    for (;;) {
        printf("# ");
        read(&buf[0], 1024);

        if (strcmp(&buf[0], "exit") == 0)
            break;
        if (strcmp(&buf[0], "memstat") == 0) {
            builtin_memstat();
            continue;
        }
        if (strcmp(&buf[0], "uptime") == 0) {
            builtin_uptime();
            continue;
        }
        if (strcmp(&buf[0], "yield") == 0) {
            yield();
            continue;
        }
        if (strcmp(&buf[0], "spin") == 0) {
            for (volatile int i = 0; i < 10'0000'000; i++) { };
            continue;
        }
        if (strcmp(&buf[0], "sleep") == 0) {
            printf("Sleeping for 2 seconds.\n");
            sleep(2);
            continue;
        }
        if (strcmp(&buf[0], "help") == 0) {
            printf("The following commands are available to you:\n");
            printf("  - memstat\tProvides statistics on the amount of memory used by this task.\n");
            printf("  - uptime\tProvides statistics on the time since boot in seconds, as well as the CPU time used by this task.\n");
            printf("  - yield\tYields to the spin task. The spin task simply spins until it is preempted. You should see control return to this task shortly.\n");
            printf("  - sleep\tPuts this task to sleep for 2 seconds.\n");
            printf("  - spin\tSpins in a loop for a couple seconds.\n");
            printf("  - exit\tSays goodbye. Please hit Ctrl-C to actually exit.\n");
            printf("\n");
            printf("Known Bugs (because we're honest around here!):\n");
            printf("  - You may encounter periods of a second or more of unresponsiveness. This is a known issue related to emulation and the design the system timer peripheral.\n");
            printf("  - 'sleep' can sleep for more than 2 seconds. Related to the above issue.\n");
            printf("  - Time slowly drifts away from real time.\n");
            continue;
        }
        printf("Unknown command '%s'. Use 'help'.\n", &buf[0]);
    }

    printf("goodbye.\n");
    exit(0);
}

u32 shell_addr() {
    u32 addr;
    asm volatile("ldr %0, =shell"
                 : "=r"(addr));
    return addr;
}

}

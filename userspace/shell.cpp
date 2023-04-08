#include "shell.hpp"
#include "lib.hpp"

#include <pine/math.hpp>
#include <pine/units.hpp>

using namespace pine;

static void builtin_memstat()
{
    auto malloc_stats = memstats();
    unsigned int pct_of_heap = (malloc_stats.used_size * 100) / malloc_stats.heap_size;
    printf("heap size: %zu bytes\n"
           "requested: %zu bytes (%u%% of heap)\n"
           "nmallocs: %u\nnfrees: %u\n",
           malloc_stats.heap_size,
           malloc_stats.used_size,
           pct_of_heap,
           malloc_stats.num_mallocs,
           malloc_stats.num_frees);
}

static void builtin_uptime()
{
    unsigned uptime_jiffies = uptime();
    unsigned uptime_seconds = uptime_jiffies >> SYS_HZ_BITS;
    unsigned cputime_jiffies = cputime();
    unsigned cpu_usage = cputime_jiffies * 100u / pine::max(uptime_jiffies, static_cast<decltype(cputime_jiffies)>(1));
    printf("up %us, usage: %u%% (%u / %u jiffies)\n", uptime_seconds, cpu_usage, cputime_jiffies, uptime_jiffies);
}

static void setup_uart_as_stdio()
{
    int uart_read_fd = open("/dev/uart0", FileMode::Read);
    assert(uart_read_fd > 0);
    int uart_write_fd = open("/dev/uart0", FileMode::Write);
    assert(uart_write_fd > 0);

    assert(close(STDIN_FILENO) >= 0);
    assert(dup(uart_read_fd) >= 0);
    assert(close(STDOUT_FILENO) >= 0);
    assert(dup(uart_write_fd) >= 0);
}

static void display_welcome()
{
    int display_fd = open("/dev/display", FileMode::ReadWrite);
    assert(display_fd > 0);

    const char* message = "Welcome to Pinyon!";
    assert(write(display_fd, message, strlen(message)) > 0);

    assert(close(display_fd) >= 0);
}

extern "C" {

void shell()
{
    setup_uart_as_stdio();  // Make /dev/uart0 stdin and stdout

    display_welcome();

    TVector<char> buf(mem_allocator());
    if (!buf.ensure(1024)) {
        printf("Could not allocate memory for buf!!\n");
        exit(1);
    }

    printf("Use 'help' for a list of commands to run.");
    for (;;) {
        printf("# ");
        ssize_t amount_read = read(STDIN_FILENO, &buf[0], 1024);
        assert(amount_read >= 0);
        pine::StringView command(buf.data(), static_cast<size_t>(amount_read));

        if (command == "exit")
            break;
        if (command == "memstat") {
            builtin_memstat();
            continue;
        }
        if (command == "uptime") {
            builtin_uptime();
            continue;
        }
        if (command == "yield") {
            yield();
            continue;
        }
        if (command == "spin") {
            for (volatile int i = 0; i < 100'000'000; i++) { };
            continue;
        }
        if (command == "sleep") {
            printf("Sleeping for 2 seconds.\n");
            sleep(2);
            continue;
        }
        if (command == "help") {
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
        printf("Unknown command '%s'. Use 'help'.\n", command.data());
    }

    printf("goodbye.\n");
    exit(0);
}

PtrData shell_addr() {
    PtrData addr;
    asm volatile("ldr %0, =shell"
                 : "=r"(addr));
    return addr;
}

}

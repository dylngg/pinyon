#include "console.hpp"
#include "kmalloc.hpp"

extern "C" [[noreturn]] void init();

static void readline(char* buf, size_t bufsize)
{
    size_t offset = 0;
    char ch;

    for (;;) {
        ch = console_get();
        if (ch == '\r')
            break;
        if (ch == '\n')
            break;

        if (offset >= bufsize - 1)
            continue;

        // For some reason local echo doesn't work on my machine, so we'll just
        // echo it here...
        console_put(ch);

        buf[offset] = ch;
        offset++;
    }

    console_put('\n');
    buf[offset] = '\0';
    return;
}

static void builtin_memstat()
{
    auto malloc_stats = kmemstats();
    int pct_mem_util = (malloc_stats.amount_used * 100) / malloc_stats.heap_size;
    consolef("heap size: %d bytes\nused: %d bytes (%d%% util)\nnmallocs: %d\nnfrees: %d\n",
        malloc_stats.heap_size, malloc_stats.amount_used, pct_mem_util, malloc_stats.num_mallocs,
        malloc_stats.num_frees);
}

static void start_shell()
{
    char* buf = (char*)kmalloc(1024);

    for (;;) {
        console("# ");
        readline(buf, 1024);

        if (strcmp(buf, "exit") == 0)
            break;
        if (strcmp(buf, "memstat") == 0) {
            builtin_memstat();
            continue;
        }
        consolef("Unknown command '%s'\n", buf);
    }

    kfree((void*)buf);
}

[[noreturn]] void init()
{
    console_init();
    consoleln("Initializing... ");
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
    while (1) {
    }
}

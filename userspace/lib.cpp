#include "lib.hpp"
#include <pine/units.hpp>

void readline(char* buf, u32 at_most_bytes)
{
    syscall_readline(buf, at_most_bytes);
}

void yield()
{
    syscall_yield();
}

void sleep(u32 secs)
{
    syscall_sleep(secs);
}

u32 uptime() {
    u32 jif;
    syscall_uptime(&jif);
    return jif;
}

u32 cputime() {
    u32 jif;
    syscall_cputime(&jif);
    return jif;
}

void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t bufsize = strlen(fmt) * 2;
    char* print_buf = (char*)malloc(bufsize);
    size_t print_buf_pos = 0;

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        size_t message_size = strlen(message);
        if (print_buf_pos + message_size >= bufsize) {
            // realloc buffer size (hack)
            // FIXME: Implement realloc
            free(print_buf);
            bufsize = bufsize + message_size + 64;
            print_buf = (char*)malloc(bufsize);
        }

        strcpy(print_buf + print_buf_pos, message);
        print_buf_pos += message_size;
        return true;
    };

    vfnprintf(try_add_wrapper, fmt, args);
    syscall_write(print_buf, print_buf_pos);
    free(print_buf);
}

void* heap_reserve()
{
    void* start_addr;
    syscall_heap_reserve(&start_addr);
    return start_addr;
}

size_t heap_incr(size_t by_bytes)
{
    size_t incr = 0;
    syscall_heap_incr(by_bytes, &incr);
    return incr;
}

TaskMemoryBounds::TaskMemoryBounds()
{
    m_heap_start = (PtrData)heap_reserve();
    m_heap_size = heap_incr(Page);
}

PtrData TaskMemoryBounds::heap_start() const
{
    return m_heap_start;
}

PtrData TaskMemoryBounds::heap_end() const
{
    return m_heap_start + m_heap_size;
}

size_t TaskMemoryBounds::try_extend_heap(size_t by_size)
{
    if (heap_start() + by_size <= heap_end()) {
        m_heap_size += by_size;
        return by_size;
    }

    // Need to allocate memory
    size_t heap_incr_size = heap_incr(by_size);
    m_heap_size += heap_incr_size;
    return heap_incr_size;
}

static TaskMemoryBounds g_task_memory_bounds;
static TaskMemoryAllocator g_task_allocator { nullptr };

void mem_init()
{
    g_task_memory_bounds = TaskMemoryBounds {};
    g_task_allocator = TaskMemoryAllocator { &g_task_memory_bounds };
}

TaskMemoryBounds& mem_bounds()
{
    return g_task_memory_bounds;
}

TaskMemoryAllocator& mem_allocator()
{
    return g_task_allocator;
}

void* malloc(size_t requested_size)
{
    void* ptr = mem_allocator().allocate(requested_size);
    if (!ptr)
        printf("kmalloc:\tNo free space available?!\n");

    return ptr;
}

void free(void* ptr)
{
    mem_allocator().free(ptr);
}

MallocStats memstats()
{
    return mem_allocator().stats();
}

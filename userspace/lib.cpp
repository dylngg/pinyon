#include "lib.hpp"

#include <pine/string.hpp>
#include <pine/units.hpp>

void readline(char* buf, u32 at_most_bytes)
{
    u32 bytes_read = syscall_read(buf, at_most_bytes - 1);
    buf[bytes_read] = '\0';
}

void yield()
{
    syscall_yield();
}

void sleep(u32 secs)
{
    syscall_sleep(secs);
}

u32 uptime()
{
    return syscall_uptime();
}

u32 cputime()
{
    return syscall_cputime();
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

        strcopy(print_buf + print_buf_pos, message);
        print_buf_pos += message_size;
        return true;
    };

    vfnprintf(try_add_wrapper, fmt, args);
    syscall_write(print_buf, print_buf_pos);
    free(print_buf);
}

void* heap_allocate()
{
    return syscall_heap_allocate();
}

size_t heap_incr(size_t by_bytes)
{
    return syscall_heap_incr(by_bytes);
}

Maybe<TaskHeapAllocator> TaskHeapAllocator::try_construct()
{
    auto* heap_start_ptr = heap_allocate();
    if (!heap_start_ptr)
        return {};

    auto heap_start = reinterpret_cast<PtrData>(heap_start_ptr);
    auto heap_size = heap_incr(Page);
    if (heap_size <= Page)
        return {};

    return { TaskHeapAllocator { heap_start, heap_start + heap_size } };
}

Pair<void*, size_t> TaskHeapAllocator::allocate(size_t requested_size)
{
    auto [ptr, size] = HighWatermarkAllocator::allocate(requested_size);
    if (!ptr) {
        size_t increase = heap_incr(align_up_two(requested_size, Page));
        HighWatermarkAllocator::extend_by(increase);
        if (increase < requested_size)
            return { nullptr, 0 };
    }

    return { ptr, size };
}

TaskHeapAllocator& heap_allocator()
{
    static TaskHeapAllocator g_maybe_task_heap_allocator = *TaskHeapAllocator::try_construct();
    return g_maybe_task_heap_allocator;
}

TaskMemoryAllocator& mem_allocator()
{
    // FIXME: We should assert that the heap allocator could be created! UB if not!
    static TaskMemoryAllocator g_task_allocator { &heap_allocator() };
    return g_task_allocator;
}

void* malloc(size_t requested_size)
{
    auto [ptr, _] = mem_allocator().allocate(requested_size);
    if (!ptr)
        printf("malloc:\tNo free space available?!\n");

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

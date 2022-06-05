#include "lib.hpp"

#include <pine/string.hpp>
#include <pine/units.hpp>

size_t read(char* buf, size_t at_most_bytes)
{
    u32 bytes_read = syscall2(Syscall::Read, reinterpret_cast<u32>(buf), at_most_bytes - 1);
    buf[bytes_read] = '\0';
    return bytes_read;
}

void write(char* buf, size_t bytes)
{
    syscall2(Syscall::Write, reinterpret_cast<u32>(buf), bytes);
}

void yield()
{
    syscall0(Syscall::Yield);
}

void sleep(u32 secs)
{
    syscall1(Syscall::Sleep, secs);
}

u32 uptime()
{
    return syscall0(Syscall::Uptime);
}

u32 cputime()
{
    return syscall0(Syscall::CPUTime);
}

void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t bufsize = strlen(fmt) * 2;
    char* print_buf = (char*)malloc(bufsize);
    if (!print_buf)
        return;

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
    write(print_buf, print_buf_pos);
    free(print_buf);
}

void* sbrk(size_t increase)
{
    return reinterpret_cast<void*>(syscall1(Syscall::Sbrk, increase));
}

Pair<void*, size_t> HeapExtender::allocate(size_t requested_size)
{
    size_t increase = align_up_two(requested_size, Page);
    void* heap_start_ptr = sbrk(increase);
    if (!heap_start_ptr)
        return {};

    return { heap_start_ptr, increase };
}

TaskMemoryAllocator& mem_allocator()
{
    static TaskMemoryAllocator g_task_allocator = TaskMemoryAllocator::construct();
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

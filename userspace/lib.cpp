#include "lib.hpp"

#include <pine/c_string.hpp>
#include <pine/units.hpp>
#include <pine/bit.hpp>
#include <pine/cast.hpp>

using namespace pine;

int open(StringView path, FileMode mode)
{
    auto arg1 = reinterpret_cast<PtrData>(path.data());
    auto arg2 = from_signed_cast<PtrData>(mode);
    auto result = syscall2(Syscall::Open, arg1, arg2);
    return to_signed_cast<int>(result);
}

ssize_t read(int fd, char* buf, size_t at_most_bytes)
{
    auto arg1 = from_signed_cast<PtrData>(fd);
    auto arg2 = reinterpret_cast<PtrData>(reinterpret_cast<void*>(buf));
    auto result = syscall3(Syscall::Read, arg1, arg2, at_most_bytes - 1);
    auto bytes_read = pine::bit_cast<ssize_t>(result);
    buf[bytes_read] = '\0';
    return bytes_read;
}

ssize_t write(int fd, const char* buf, size_t bytes)
{
    auto arg1 = from_signed_cast<PtrData>(fd);
    auto arg2 = reinterpret_cast<PtrData>(reinterpret_cast<const void*>(buf));
    return pine::bit_cast<ssize_t>(syscall3(Syscall::Write, arg1, arg2, bytes));
}

int close(int fd)
{
    auto arg1 = from_signed_cast<PtrData>(fd);
    auto result = syscall1(Syscall::Close, arg1);
    return to_signed_cast<int>(result);
}

int dup(int fd)
{
    auto arg1 = from_signed_cast<PtrData>(fd);
    auto result = syscall1(Syscall::Dup, arg1);
    return to_signed_cast<int>(result);
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
    return static_cast<u32>(syscall0(Syscall::Uptime));
}

u32 cputime()
{
    return static_cast<u32>(syscall0(Syscall::CPUTime));
}

int printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t bufsize = strlen(fmt) * 2;
    auto print_alloc = malloc(bufsize);
    if (!print_alloc)
        return 0;

    char* print_buf = static_cast<char*>(print_alloc.ptr);
    size_t print_buf_pos = 0;

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        size_t message_size = strlen(message);
        if (print_buf_pos + message_size >= bufsize) {
            // realloc buffer size (hack)
            // FIXME: Implement realloc
            free(print_alloc);
            bufsize = bufsize + message_size + 64;
            print_alloc = malloc(bufsize);
            if (!print_alloc)
                return false;
        }

        strcopy(print_buf + print_buf_pos, message);
        print_buf_pos += message_size;
        return true;
    };

    vfnprintf(try_add_wrapper, fmt, args);
    write(STDOUT_FILENO, print_buf, print_buf_pos);
    free(print_alloc);
    return 0;
}

static MallocStats g_malloc_stats;

void* sbrk(size_t increase)
{
    return reinterpret_cast<void*>(syscall1(Syscall::Sbrk, increase));
}

Pair<void*, size_t> HeapExtender::allocate(size_t requested_size)
{
    size_t increase = align_up_two(requested_size, PageSize);
    void* heap_start_ptr = sbrk(increase);
    if (!heap_start_ptr)
        return {};

    g_malloc_stats.heap_size += increase;
    return { heap_start_ptr, increase };
}

TaskMemoryAllocator& mem_allocator()
{
    static HeapExtender g_heap_extender {};
    static TaskMemoryAllocator g_task_allocator = TaskMemoryAllocator(&g_heap_extender);
    return g_task_allocator;
}

pine::Allocation malloc(size_t requested_size)
{
    auto alloc = mem_allocator().allocate(requested_size);
    if (!alloc)
        printf("malloc:\tNo free space available?!\n");

    g_malloc_stats.used_size += alloc.size;
    ++g_malloc_stats.num_mallocs;
    return alloc;
}

void free(pine::Allocation alloc)
{
    g_malloc_stats.used_size -= mem_allocator().free(alloc);
    ++g_malloc_stats.num_frees;
}

MallocStats memstats()
{
    return g_malloc_stats;
}

void exit(int code)
{
    syscall1(Syscall::Exit, pine::bit_cast<PtrData>(static_cast<ptrdiff_t>(code)));
}

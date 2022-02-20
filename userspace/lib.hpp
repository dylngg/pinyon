#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/printf.hpp>
#include <pine/types.hpp>

// See syscall.S
extern "C" {
void syscall_yield();
void syscall_sleep(u32 secs);
u32 syscall_read(char* buf, u32 bytes);
void syscall_write(const char* buf, u32 bytes);
void* syscall_heap_allocate();
size_t syscall_heap_incr(size_t by_bytes);
u32 syscall_uptime();
u32 syscall_cputime();
}

void readline(char* buf, u32 bytes);

void yield();

void sleep(u32 secs);

u32 uptime();

u32 cputime();

void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

class TaskHeapAllocator : public HighWatermarkAllocator {
public:
    static Maybe<TaskHeapAllocator> try_construct();

    Pair<void*, size_t> allocate(size_t requested_size); // override

private:
    using HighWatermarkAllocator::HighWatermarkAllocator;
};

using TaskMemoryAllocator = MemoryAllocator<TaskHeapAllocator, FreeList>;

TaskHeapAllocator& heap_allocator();
TaskMemoryAllocator& mem_allocator();

void free(void*);

void* malloc(size_t) __attribute__((malloc));

MallocStats memstats();

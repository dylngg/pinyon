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

class TaskMemoryBounds {
public:
    TaskMemoryBounds();

    Maybe<size_t> try_extend_heap(size_t by_size);
    PtrData heap_start() const;
    PtrData heap_end() const;

    bool in_bounds(void* ptr) const;

private:
    PtrData m_heap_start;
    PtrData m_heap_size;
};

using TaskMemoryAllocator = MemoryAllocator<TaskMemoryBounds, FreeList>;

void free(void*);

void* malloc(size_t) __attribute__((malloc));

MallocStats memstats();
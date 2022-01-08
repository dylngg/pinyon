#pragma once

#include <pine/malloc.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>

// See syscall.S
extern "C" {
void syscall_yield();
void syscall_sleep(u32 secs);
void syscall_readline(char* buf, u32 bytes);
void syscall_write(const char* buf, u32 bytes);
void syscall_heap_reserve(void** start_addr);
void syscall_heap_incr(size_t by_bytes, size_t* incr_size);
void syscall_uptime(u32* jiffies);
void syscall_cputime(u32* jiffies);
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
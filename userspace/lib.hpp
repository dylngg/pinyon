#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/printf.hpp>
#include <pine/types.hpp>
#include <pine/syscall.hpp>

// See syscall.S
extern "C" {
u32 syscall0(Syscall call);
u32 syscall1(Syscall call, u32 arg1);
u32 syscall2(Syscall call, u32 arg1, u32 arg2);
}

size_t read(char* buf, u32 bytes);

void write(char* buf, u32 bytes);

void yield();

void sleep(u32 secs);

u32 uptime();

u32 cputime();

void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

struct HeapExtender {
    static HeapExtender construct() { return {}; }

    Pair<void*, size_t> allocate(size_t);
    void free(void*);
    bool in_bounds(void*) { return true; };
};

using TaskMemoryAllocator = MemoryAllocator<HeapExtender, FreeList>;

TaskMemoryAllocator& mem_allocator();

void free(void*);

void* malloc(size_t) __attribute__((malloc));

MallocStats memstats();

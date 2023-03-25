#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/print.hpp>
#include <pine/types.hpp>
#include <pine/syscall.hpp>
#include <pine/vector.hpp>
#include <pine/string_view.hpp>

#define assert(cond) do { if (!(cond)) { printf("Assertion failed: " #cond); exit(1); } } while(0);

// See syscall.S
extern "C" {
u32 syscall0(Syscall call);
u32 syscall1(Syscall call, u32 arg1);
u32 syscall2(Syscall call, u32 arg1, u32 arg2);
u32 syscall3(Syscall call, u32 arg1, u32 arg2, u32 arg3);
}

constexpr int STDIN_FILENO = 0;
constexpr int STDOUT_FILENO = 1;

int open(pine::StringView path, FileMode mode);

ssize_t read(int fd, char* buf, size_t bytes);

ssize_t write(int fd, const char* buf, size_t bytes);

int close(int fd);

int dup(int fd);

void yield();

void sleep(u32 secs);

u32 uptime();

u32 cputime();

int printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

struct HeapExtender {
    static HeapExtender construct() { return {}; }

    static constexpr size_t preferred_size(size_t requested_size)
    {
        return pine::align_up_two(requested_size, PageSize);
    }

    Pair<void*, size_t> allocate(size_t);
    void free(void*);
};

using TaskMemoryAllocator = pine::FallbackAllocatorBinder<HeapExtender, pine::IntrusiveFreeList>;

TaskMemoryAllocator& mem_allocator();

void free(pine::Allocation);

pine::Allocation malloc(size_t);
pine::Allocation malloc(size_t);

struct MallocStats {
    size_t heap_size = 0;
    size_t used_size = 0;
    unsigned int num_mallocs = 0;
    unsigned int num_frees = 0;
};

MallocStats memstats();

void exit(int code);

// We don't use this in any capacity, but compilers will insert calls to it
inline int atexit(void (*)()) { return 0; };

template <typename Value>
using TVector = pine::Vector<Value, TaskMemoryAllocator>;

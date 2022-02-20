#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

/*
 * If we get virtual memory working I estimate we'll keep
 * our page tables, stack, etc under this mark...
 *
 * Stacks end at 0x3EE00000 (devices start at 0x3F000000), so avoid going past that.
 */
#define HEAP_START 0x00440000
#define HEAP_END 0x3EE00000

class KernelMemoryAllocator : public MemoryAllocator<HighWatermarkAllocator, FreeList> {
public:
    using MemoryAllocator::MemoryAllocator;
    static KernelMemoryAllocator& allocator();
};

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

template <class Value>
using KOwner = Owner<Value, KernelMemoryAllocator>;

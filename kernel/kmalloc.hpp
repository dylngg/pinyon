#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

class KernelMemoryAllocator : public MemoryAllocator<HighWatermarkAllocator, FreeList> {
public:
    using MemoryAllocator::MemoryAllocator;
    static KernelMemoryAllocator& allocator();
};

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

template <class Value>
using KOwner = Owner<Value, KernelMemoryAllocator>;

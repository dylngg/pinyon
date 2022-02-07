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

class KernelMemoryBounds {
public:
    KernelMemoryBounds(PtrData heap_start, PtrData heap_end_bound);

    Maybe<size_t> try_extend_heap(size_t by_size);
    Maybe<PtrData> try_reserve_topdown_space(size_t stack_size);
    PtrData heap_start() const;
    PtrData heap_end() const;

    bool in_bounds(void* ptr) const;

private:
    PtrData m_heap_start;
    PtrData m_heap_size;
    PtrData m_heap_end_bound;
};

KernelMemoryBounds& kmem_bounds();

class KernelMemoryAllocator;
KernelMemoryAllocator& kmem_allocator();

class KernelMemoryAllocator : public MemoryAllocator<KernelMemoryBounds, FreeList> {
public:
    using MemoryAllocator::MemoryAllocator;

    static KernelMemoryAllocator& allocator()
    {
        return kmem_allocator();
    }
};

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

MallocStats kmemstats();

template <class Value>
using KOwner = Owner<Value, KernelMemoryAllocator>;

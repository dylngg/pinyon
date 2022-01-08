#pragma once
#include <pine/malloc.hpp>
#include <pine/types.hpp>

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

using KernelMemoryAllocator = MemoryAllocator<KernelMemoryBounds, FreeList>;

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

MallocStats kmemstats();

KernelMemoryBounds& kmem_bounds();
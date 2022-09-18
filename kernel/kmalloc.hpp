#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>
#include <pine/vector.hpp>

class KernelMemoryAllocator : public pine::MemoryAllocator<pine::FixedAllocation, pine::FreeList> {
public:
    using MemoryAllocator::MemoryAllocator;

    // We need to redefine this so that we can return a KernelMemoryAllocator, not a MemoryAllocator<>
    template <class... Args>
    static KernelMemoryAllocator construct(Args&&... args)
    {
        static auto sub_allocator = pine::FixedAllocation::construct(pine::forward<Args>(args)...);
        return KernelMemoryAllocator { &sub_allocator };
    }

    static KernelMemoryAllocator& allocator();
};

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

template <class Value>
using KOwner = pine::Owner<Value, KernelMemoryAllocator>;

template <class Value>
using KVector = pine::Vector<Value, KernelMemoryAllocator>;

#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>
#include <pine/vector.hpp>

using KernelMemoryAllocator = pine::MemoryAllocator<pine::FixedAllocation, pine::FreeList>;

KernelMemoryAllocator& kernel_allocator();

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

template <class Value>
using KOwner = pine::Owner<Value, KernelMemoryAllocator>;

template <class Value>
using KVector = pine::Vector<Value, KernelMemoryAllocator>;

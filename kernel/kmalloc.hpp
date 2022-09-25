#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>
#include <pine/vector.hpp>

using KernelMemoryAllocator = pine::FallbackAllocator<pine::FixedAllocation, pine::IntrusiveFreeList>;

KernelMemoryAllocator& kernel_allocator();

void kfree(pine::Allocation);

pine::Allocation kmalloc(size_t);

template <class Value>
using KOwner = pine::Owner<Value, KernelMemoryAllocator>;

template <class Value>
using KVector = pine::Vector<Value, KernelMemoryAllocator>;

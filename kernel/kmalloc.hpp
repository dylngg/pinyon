#pragma once

#include <pine/malloc.hpp>
#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/string.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>
#include <pine/vector.hpp>

#ifdef AARCH64
#elif AARCH32
#include "arch/aarch32/mmu.hpp"
#else
#error Architecture not defined
#endif

#ifdef AARCH64
using KernelMemoryAllocator = pine::FallbackAllocatorBinder<pine::FixedAllocation, pine::IntrusiveFreeList>;
#else
using KernelMemoryAllocator = pine::FallbackAllocatorBinder<mmu::PageAllocator, pine::IntrusiveFreeList>;
#endif

KernelMemoryAllocator& kernel_allocator();

void kfree(pine::Allocation);

pine::Allocation kmalloc(size_t);

template <class Value>
using KOwner = pine::Owner<Value, KernelMemoryAllocator>;

template <class Value>
using KVector = pine::Vector<Value, KernelMemoryAllocator>;

using KString = pine::String<KernelMemoryAllocator>;

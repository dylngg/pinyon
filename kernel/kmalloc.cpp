#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/barrier.hpp>

#include "mmu.hpp"

KernelMemoryAllocator& KernelMemoryAllocator::allocator()
{
    static auto g_kernel_memory_allocator = KernelMemoryAllocator::construct(HEAP_START, HEAP_END);
    return g_kernel_memory_allocator;
}

void* kmalloc(size_t requested_size)
{
    auto [ptr, _] = KernelMemoryAllocator::allocator().allocate(requested_size);
    return ptr;
}

void kfree(void* ptr)
{
    KernelMemoryAllocator::allocator().free(ptr);
}

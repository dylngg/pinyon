#include "kmalloc.hpp"
#include "mmu.hpp"

#include <pine/barrier.hpp>

KernelMemoryAllocator& kernel_allocator()
{
    static auto g_kernel_memory_allocator = KernelMemoryAllocator::construct(HEAP_START, HEAP_END);
    return g_kernel_memory_allocator;
}

void* kmalloc(size_t requested_size)
{
    auto [ptr, _] = kernel_allocator().allocate(requested_size);
    return ptr;
}

void kfree(void* ptr)
{
    kernel_allocator().free(ptr);
}

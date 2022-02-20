#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/barrier.hpp>


KernelMemoryAllocator& KernelMemoryAllocator::allocator()
{
    static HighWatermarkAllocator g_kernel_heap_allocator { HEAP_START, HEAP_END };
    static KernelMemoryAllocator g_kernel_memory_allocator { &g_kernel_heap_allocator };
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

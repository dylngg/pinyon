#include "kmalloc.hpp"
#include "mmu.hpp"

#include <pine/barrier.hpp>

KernelMemoryAllocator& kernel_allocator()
{
    static auto g_kernel_memory_allocator = KernelMemoryAllocator(&mmu::page_allocator());
    return g_kernel_memory_allocator;
}

pine::Allocation kmalloc(size_t requested_size)
{
    return kernel_allocator().allocate(requested_size);
}

void kfree(pine::Allocation alloc)
{
    kernel_allocator().free(alloc);
}

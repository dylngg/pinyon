#include "kmalloc.hpp"

#include <pine/units.hpp>

extern size_t __code_end;

KernelMemoryAllocator& kernel_allocator()
{
#ifdef AARCH64
    static auto g_fixed_kernel_memory_allocator = pine::FixedAllocation(pine::align_up_two(reinterpret_cast<size_t>(&__code_end), PageSize), 32 * MiB);
    static auto g_kernel_memory_allocator = KernelMemoryAllocator(&g_fixed_kernel_memory_allocator);
#elif AARCH32
    static auto g_kernel_memory_allocator = KernelMemoryAllocator(&mmu::page_allocator());
#else
#error Architecture not defined
#endif
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

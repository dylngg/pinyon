#include "kmalloc.hpp"

#include <pine/units.hpp>

KernelMemoryAllocator& kernel_allocator()
{
#ifdef AARCH64
    extern size_t __code_end;
    static auto g_fixed_kernel_memory_allocator = pine::FixedAllocation(align_up_two(__code_end, PageSize), 16u*PageSize);
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

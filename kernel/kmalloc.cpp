#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/barrier.hpp>

KernelMemoryBounds::KernelMemoryBounds(PtrData heap_start, PtrData heap_end_bound)
{
    m_heap_start = heap_start;
    m_heap_size = 0;
    m_heap_end_bound = heap_end_bound;
}

PtrData KernelMemoryBounds::heap_start() const
{
    return m_heap_start;
}

PtrData KernelMemoryBounds::heap_end() const
{
    return m_heap_start + m_heap_size;
}

bool KernelMemoryBounds::in_bounds(void* ptr) const
{
    auto ptr_data = reinterpret_cast<PtrData>(ptr);
    return ptr_data >= heap_start() && ptr_data < heap_end();
}

Maybe<size_t> KernelMemoryBounds::try_extend_heap(size_t by_size)
{
    if (heap_start() + by_size <= m_heap_end_bound) {
        m_heap_size += by_size;
        return { by_size };
    }

    // Cannot allocate memory!
    return {};
}

Maybe<PtrData> KernelMemoryBounds::try_reserve_topdown_space(size_t stack_size)
{
    if (m_heap_end_bound - stack_size < heap_end()) {
        return {};
    }

    PtrData stack_start = m_heap_end_bound;
    m_heap_end_bound -= stack_size;
    return { stack_start };
}

void* kmalloc(size_t requested_size)
{
    void* ptr = KernelMemoryAllocator::allocator().allocate(requested_size);
    if (!ptr)
        panic("kmalloc:\tNo free memory space available?!\n");

    return ptr;
}

void kfree(void* ptr)
{
    KernelMemoryAllocator::allocator().free(ptr);
}

KernelMemoryBounds& kmem_bounds()
{
    static KernelMemoryBounds g_kernel_memory_bounds { HEAP_START, HEAP_END };
    return g_kernel_memory_bounds;
}

KernelMemoryAllocator& kmem_allocator()
{
    static KernelMemoryAllocator g_kernel_allocator { kmem_bounds() };
    return g_kernel_allocator;
}

MallocStats kmemstats()
{
    return KernelMemoryAllocator::allocator().stats();
}

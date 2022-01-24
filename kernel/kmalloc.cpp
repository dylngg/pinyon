#include "kmalloc.hpp"
#include "panic.hpp"

#include <pine/barrier.hpp>


/*
 * If we get virtual memory working I estimate we'll keep
 * our page tables, stack, etc under this mark...
 *
 * Stacks end at 0x3EE00000 (devices start at 0x3F000000), so avoid going past that.
 */
#define HEAP_START 0x00440000
#define HEAP_END 0x3EE00000

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

void* kmalloc(size_t requested_size)
{
    void* ptr = kmem_allocator().allocate(requested_size);
    if (!ptr)
        panic("kmalloc:\tNo free memory space available?!\n");

    return ptr;
}

void kfree(void* ptr)
{
    kmem_allocator().free(ptr);
}

MallocStats kmemstats()
{
    return kmem_allocator().stats();
}

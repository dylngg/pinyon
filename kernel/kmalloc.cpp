#include "kmalloc.hpp"
#include "panic.hpp"
#include <pine/barrier.hpp>
#include <pine/types.hpp>

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

Maybe<size_t> KernelMemoryBounds::try_extend_heap(size_t by_size)
{
    if (heap_start() + by_size <= m_heap_end_bound) {
        m_heap_size += by_size;
        return Maybe<size_t>(by_size);
    }

    // Cannot allocate memory!
    return Maybe<size_t>::No();
}

Maybe<PtrData> KernelMemoryBounds::try_reserve_topdown_space(size_t stack_size)
{
    if (m_heap_end_bound - stack_size < heap_end()) {
        return Maybe<PtrData>::No();
    }

    PtrData stack_start = m_heap_end_bound;
    m_heap_end_bound -= stack_size;
    return Maybe<PtrData>(stack_start);
}

static KernelMemoryBounds g_kernel_memory_bounds { 0, 0 };
static KernelMemoryAllocator g_kernel_allocator { &g_kernel_memory_bounds };

KernelMemoryBounds& kmem_bounds()
{
    return g_kernel_memory_bounds;
}

KernelMemoryAllocator& kmem_allocator()
{
    return g_kernel_allocator;
}

void kmem_init()
{
    g_kernel_memory_bounds = KernelMemoryBounds { HEAP_START, HEAP_END };
    g_kernel_allocator = KernelMemoryAllocator { &g_kernel_memory_bounds };
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

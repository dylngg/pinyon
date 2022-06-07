#pragma once
#include "c_builtins.hpp"
#include "linked_list.hpp"
#include "twomath.hpp"
#include "types.hpp"
#include "units.hpp"

namespace pine {

/*
 * Memory allocation here is done in a generic templated manner, allowing for
 * reuse between userspace and the kernel. Allocation is controlled with the
 * MemoryAllocator class, which is another memory allocator that returns memory
 * that this allocator can subdivide. The particular memory management system
 * to use (e.g. free list, buddy allocation, etc) is controlled with the
 * MemoryManager class.
 *
 * This clever subdivision allocation scheme is based off of a talk by Andrei
 * Alexandrescu at CppCon:
 * https://www.youtube.com/watch?v=LIb3L4vKZ7U
 */

struct Allocation {
    void* ptr = nullptr;
    size_t size = 0;
};

class FreeList {
    // Wrap size in padded struct to ensure alignment.
    // e.g. ManualLinkedList<size_t>::Node on ARM32 is 12 bytes, which is not 8
    //      byte aligned
    struct Header {
        alignas(max_align_t) size_t size;
    };
    using HeaderNode = ManualLinkedList<Header>::Node;

public:
    FreeList() = default;

    static size_t preferred_size(size_t requested_size)
    {
        return align_up_two(min_allocation_size(requested_size), PageSize);
    }

    Allocation try_reserve(size_t);
    void add(void*, size_t);
    size_t release(void*);

private:
    constexpr static size_t min_allocation_size(size_t requested_size)
    {
        size_t alloc_size = align_up_two(requested_size, Alignment) + sizeof(HeaderNode);
        static_assert(is_aligned_two(sizeof(HeaderNode), Alignment), "Free list header size is not aligned!");
        return alloc_size;
    }
    HeaderNode* find_first_free_node(size_t);
    Pair<HeaderNode*, HeaderNode*> try_find_neighboring_memory_nodes(HeaderNode* node_ptr);

    static void* to_user_ptr(HeaderNode* node_ptr, size_t offset = 0);
    static HeaderNode* to_node_ptr(void* addr);
    static void adopt_right_node_size(HeaderNode* left_node_ptr, HeaderNode* right_node_ptr);
    static HeaderNode* construct_node(size_t region_size, void* node_location);

    ManualLinkedList<Header> m_free_list;
};

/*
 * Allocates memory from the SubMemoryAllocator and manages it with the
 * MemoryManager.
 */
template <class SubMemoryAllocator, class MemoryManager>
class MemoryAllocator {
public:
    MemoryAllocator(SubMemoryAllocator* mem_allocator)
        : m_allocator(mem_allocator)
        , m_manager() {};

    template <class... Args>
    static MemoryAllocator construct(Args&&... args)
    {
        static auto sub_allocator = SubMemoryAllocator::construct(forward<Args>(args)...);
        return MemoryAllocator<SubMemoryAllocator, MemoryManager> { &sub_allocator };
    }

    Pair<void*, size_t> allocate(size_t requested_size)
    {
        auto allocation = m_manager.try_reserve(requested_size);
        if (!allocation.ptr) {
            auto preferred_size = MemoryManager::preferred_size(requested_size);
            auto [allocated_ptr, allocated_size] = m_allocator->allocate(preferred_size);
            if (!allocated_ptr)
                return { nullptr, 0 };

            m_manager.add(allocated_ptr, allocated_size);
            allocation = m_manager.try_reserve(requested_size);
        }

        return { allocation.ptr, allocation.size };
    }

    size_t free(void* ptr)
    {
        if (!ptr || !m_allocator->in_bounds(ptr))
            return 0; // FIXME: assert?!

        size_t size_freed = m_manager.release(ptr);
        return size_freed;
    }

    bool in_bounds(void* ptr) const
    {
        return m_allocator->in_bounds(ptr);
    }

private:
    SubMemoryAllocator* m_allocator;
    MemoryManager m_manager;
};

class HighWatermarkManager {
public:
    static size_t preferred_size(size_t requested_size) { return requested_size; }

    Allocation try_reserve(size_t requested_size);
    void add(void* ptr, size_t size);
    size_t release(void*) { return 0; }

private:
    u8* m_start = 0;
    u8* m_watermark = 0;
    u8* m_end = 0;
};

class FixedAllocation {
public:
    FixedAllocation(PtrData start, size_t size)
        : m_start(start)
        , m_size(size)
        , m_has_memory(true) {};

    static FixedAllocation construct(PtrData start, size_t size) { return { start, size }; }

    Pair<void*, size_t> allocate(size_t requested_size);
    size_t free(void*);
    bool in_bounds(void* ptr) const;

private:
    PtrData m_start;
    size_t m_size;
    bool m_has_memory;
};

using FixedHeapAllocator = MemoryAllocator<FixedAllocation, HighWatermarkManager>;

}

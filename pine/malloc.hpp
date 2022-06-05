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

struct MallocStats {
    size_t heap_size = 0;
    size_t amount_requested = 0;
    size_t amount_allocated = 0;
    u32 num_mallocs = 0;
    u32 num_frees = 0;
};

struct AllocationStats {
    size_t requested_size = 0;
    size_t allocated_size = 0;
};

class FreeList {

    // Stores the size of the memory region belonging to it (inferred from
    // 'this' pointer)
    struct SizeData {
        explicit SizeData(size_t requested_size, size_t reserved_size)
            : m_requested_size(requested_size)
            , m_reserved_size(reserved_size) {};
        explicit SizeData(size_t reserved_size)
            : m_requested_size(0)
            , m_reserved_size(reserved_size) {};

        void reserve(size_t requested_size) { m_requested_size = requested_size; }
        void shrink_by(size_t size) { m_reserved_size -= size; }
        void grow_by(size_t size) { m_reserved_size += size; }
        size_t requested_size() const { return m_requested_size; }
        size_t reserved_size() const { return m_reserved_size; }
        size_t remaining_size() const { return reserved_size() - requested_size(); }

    private:
        size_t m_requested_size;
        size_t m_reserved_size;
    };
    using SizeNode = LinkedList<SizeData>::Node;

public:
    FreeList() = default;

    static size_t preferred_size(size_t requested_size)
    {
        return align_up_two(allocation_size(requested_size), PageSize);
    }

    Pair<void*, AllocationStats> try_reserve(size_t);
    void add(void*, size_t);
    AllocationStats release(void*);

private:
    constexpr static size_t allocation_size(size_t requested_size)
    {
        size_t alloc_size = align_up_two(requested_size, Alignment) + sizeof(SizeNode);
        static_assert(is_aligned_two(sizeof(SizeNode), Alignment), "Free list header size is not aligned!");
        return alloc_size;
    }
    SizeNode* find_first_free_node(size_t);
    Pair<SizeNode*, SizeNode*> try_find_neighboring_memory_nodes(SizeNode* node_ptr);

    static void* user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset = 0);
    static SizeNode* node_ptr_from_user_addr(void* addr);
    static bool nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static void adopt_right_node_size(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static SizeNode* construct_node(size_t region_size, void* node_location);

    LinkedList<SizeData> m_free_list;
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
        , m_manager()
        , m_stats() {};

    template <class... Args>
    static MemoryAllocator construct(Args&&... args)
    {
        static auto sub_allocator = SubMemoryAllocator::construct(forward<Args>(args)...);
        return MemoryAllocator<SubMemoryAllocator, MemoryManager> { &sub_allocator };
    }

    Pair<void*, size_t> allocate(size_t requested_size)
    {
        auto [ptr, alloc_stats] = m_manager.try_reserve(requested_size);
        if (!ptr) {
            auto preferred_size = MemoryManager::preferred_size(requested_size);
            auto [allocated_ptr, allocated_size] = m_allocator->allocate(preferred_size);
            if (!allocated_ptr)
                return { nullptr, 0 };

            m_manager.add(allocated_ptr, allocated_size);
            m_stats.heap_size += allocated_size;

            auto ptr_and_alloc_stats = m_manager.try_reserve(requested_size);
            ptr = ptr_and_alloc_stats.first;
            alloc_stats = ptr_and_alloc_stats.second;
        }

        m_stats.amount_requested += alloc_stats.requested_size;
        m_stats.amount_allocated += alloc_stats.allocated_size;
        m_stats.num_mallocs += ptr ? 1 : 0;

        return { ptr, requested_size };
    }

    void free(void* ptr)
    {
        if (!ptr || !m_allocator->in_bounds(ptr))
            return; // FIXME: assert?!

        auto [requested_size, freed_size] = m_manager.release(ptr);
        m_stats.amount_requested -= requested_size;
        m_stats.amount_allocated -= freed_size;
        m_stats.num_frees++;
    }

    bool in_bounds(void* ptr) const
    {
        return m_allocator->in_bounds(ptr);
    }

    MallocStats stats() const { return m_stats; }

private:
    SubMemoryAllocator* m_allocator;
    MemoryManager m_manager;
    MallocStats m_stats;
};

class HighWatermarkManager {
public:
    static size_t preferred_size(size_t requested_size) { return requested_size; }

    Pair<void*, AllocationStats> try_reserve(size_t requested_size);
    void add(void* ptr, size_t size);
    AllocationStats release(void*) { return { 0, 0}; }

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
    void free(void*);
    bool in_bounds(void* ptr) const;

private:
    PtrData m_start;
    size_t m_size;
    bool m_has_memory;
};

using FixedHeapAllocator = MemoryAllocator<FixedAllocation, HighWatermarkManager>;

}

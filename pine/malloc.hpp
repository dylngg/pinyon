#pragma once
#include "c_builtins.hpp"
#include "linked_list.hpp"
#include "twomath.hpp"
#include "types.hpp"

/*
 * Memory allocation here is done in a generic templated manner, allowing for
 * reuse between userspace and the kernel. Allocation is controlled with the
 * MemoryAllocator class, which takes in a MemoryBounds singleton that
 * controls the memory bounds of defined memory range, as well as a
 * MemoryManager template that defines the particular memory management system
 * to use (e.g. free list, buddy allocation, etc).
 *
 * Right now a generic first fit free-list memory management scheme is used by
 * both userspace and the kernel. This is not the best, but it works well
 * enough to kick the can of replacing it down the road...
 */

#define NEW_BLOCK_SIZE ((size_t)4096)
#define ALIGNMENT_SIZE ((size_t)4)

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

        constexpr size_t static preferred_alignment() { return ALIGNMENT_SIZE; }
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

    static size_t heap_increase_size(size_t requested_size)
    {
        return align_up_two(allocation_size(requested_size), NEW_BLOCK_SIZE);
    }

    Pair<void*, AllocationStats> try_find_memory(size_t size);
    void add_memory(void* new_location, size_t new_size);
    AllocationStats free_memory(void* ptr);

private:
    constexpr static size_t allocation_size(size_t requested_size)
    {
        size_t alloc_size = align_up_two(requested_size, ALIGNMENT_SIZE) + sizeof(SizeNode);
        static_assert(is_aligned_two(sizeof(SizeNode), ALIGNMENT_SIZE), "Free list header size is not aligned!");
        return alloc_size;
    }
    constexpr static size_t min_allocation_size()
    {
        return allocation_size(ALIGNMENT_SIZE);
    }

    static void* user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset = 0);
    static SizeNode* node_ptr_from_user_addr(void* addr);
    static bool nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static void adopt_right_node_size(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static SizeNode* construct_node(size_t region_size, void* node_location);

    Pair<SizeNode*, SizeNode*> try_find_neighboring_memory_nodes(SizeNode* node_ptr);

    LinkedList<SizeData> m_free_list;
};

/*
 * Defines a wrapper class which simply acts as a middle man between the given
 * MemoryBounds singleton (which defines the bounds of a memory range) and the
 * particular MemoryManager defined (which allows for different allocation
 * schemes such as free lists, buddy system, etc...).
 */
template <class MemoryBounds, class MemoryManager>
class MemoryAllocator {
public:
    MemoryAllocator(MemoryBounds& mem_bounds)
        : m_mem_bounds(mem_bounds)
        , m_memory_manager()
        , m_stats() {};

    void* allocate(size_t requested_size)
    {
        auto [free_block, alloc_stats] = m_memory_manager.try_find_memory(requested_size);
        if (!free_block) {
            auto requested_incr_size = MemoryManager::heap_increase_size(requested_size);
            auto old_heap_end = reinterpret_cast<void*>(m_mem_bounds.heap_end());
            auto maybe_heap_incr_size = m_mem_bounds.try_extend_heap(requested_incr_size);
            if (!maybe_heap_incr_size) // could not even increase a bit
                return nullptr;

            auto heap_incr_size = maybe_heap_incr_size.value();
            m_stats.heap_size += heap_incr_size;
            m_memory_manager.add_memory(old_heap_end, heap_incr_size);

            // The actual heap increase may not exactly match the memory
            // manager's requested heap increase, so it is possible that
            // additional memory is allocated on the heap without fufilling the
            // requested size
            auto block_and_size = m_memory_manager.try_find_memory(requested_size);
            free_block = block_and_size.first;
            alloc_stats = block_and_size.second;
        }

        m_stats.amount_requested += alloc_stats.requested_size;
        m_stats.amount_allocated += alloc_stats.allocated_size;
        m_stats.num_mallocs += free_block ? 1 : 0;

        return free_block;
    }

    void free(void* ptr)
    {
        if (!ptr || !m_mem_bounds.in_bounds(ptr))
            return; // FIXME: assert?!

        auto [requested_size, freed_size] = m_memory_manager.free_memory(ptr);
        m_stats.amount_requested -= requested_size;
        m_stats.amount_allocated -= freed_size;
        m_stats.num_frees++;
    }

    MallocStats stats() const { return m_stats; };

private:
    MemoryBounds& m_mem_bounds;
    MemoryManager m_memory_manager;
    MallocStats m_stats;
};

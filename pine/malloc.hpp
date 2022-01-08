#pragma once
#include "linked_list.hpp"
#include "twomath.hpp"
#include "types.hpp"

/*
 * Memory allocation here is done in a generic templated manner, allowing for
 * reuse between userspace and the kernel. Allocation is controlled with the
 * MemoryAllocator class, which takes in a MemoryBounds singleton that
 * controls the memory bounds of defined memory range, as well as a
 * SpaceManager template that defines the particular space management system
 * to use (e.g. free list, buddy allocation, etc).
 *
 * Right now a generic first fit free-list space management scheme is used by
 * both userspace and the kernel. This is not the best, but it works well
 * enough to kick the can of replacing it down the road...
 */

#define NEW_BLOCK_SIZE ((size_t)4096)
#define MIN_FREE_LIST_SIZE ((size_t)8)

struct MallocStats {
    size_t heap_size = 0;
    size_t amount_used = 0;
    u64 num_mallocs = 0;
    u64 num_frees = 0;
};

class FreeList {
    using SizeNode = LinkedList<size_t>::Node;

public:
    FreeList() = default;

    static size_t new_allocation_size(size_t requested_size)
    {
        return align_up_two(requested_size + sizeof(SizeNode), NEW_BLOCK_SIZE);
    }

    Pair<void*, size_t> try_find_memory(size_t size);
    void add_memory(void* new_location, size_t new_size);
    size_t free_memory(void* ptr);

private:
    static void* user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset = 0);
    static SizeNode* node_ptr_from_user_addr(void* addr);
    static bool nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static void adopt_right_node_size(SizeNode* left_node_ptr, SizeNode* right_node_ptr);
    static SizeNode* construct_node(size_t region_size, void* node_location);

    Pair<SizeNode*, SizeNode*> try_find_neighboring_memory_nodes(SizeNode* node_ptr);

    LinkedList<size_t> m_free_list;
};

/*
 * Defines a wrapper class which simply acts as a middle man between the given
 * MemoryBounds singleton (which defines the bounds of a memory range) and the
 * particular SpaceManager defined (which allows for different allocation
 * schemes such as free lists, buddy system, etc...).
 */
template <class MemoryBounds, class SpaceManager>
class MemoryAllocator {
public:
    MemoryAllocator(MemoryBounds& mem_bounds)
        : m_mem_bounds(mem_bounds)
        , m_space_manager()
        , m_stats() {};

    void* allocate(size_t requested_size)
    {
        auto [free_block, size_allocated] = m_space_manager.try_find_memory(requested_size);
        if (!free_block) {
            auto extend_size = SpaceManager::new_allocation_size(requested_size);
            auto old_heap_end = reinterpret_cast<void*>(m_mem_bounds.heap_end());
            auto maybe_heap_incr_size = m_mem_bounds.try_extend_heap(extend_size);
            if (!maybe_heap_incr_size)
                return nullptr;

            auto heap_incr_size = maybe_heap_incr_size.value();
            m_stats.heap_size += heap_incr_size;
            m_space_manager.add_memory(old_heap_end, heap_incr_size);

            auto block_and_size = m_space_manager.try_find_memory(requested_size);
            free_block = block_and_size.first;
            size_allocated = block_and_size.second;
        }

        m_stats.amount_used += size_allocated;
        m_stats.num_mallocs += free_block ? 1 : 0;

        return free_block;
    }

    void free(void* ptr)
    {
        if (!m_mem_bounds.in_bounds(ptr))
            return; // FIXME: assert?!

        size_t size_freed = m_space_manager.free_memory(ptr);
        m_stats.amount_used -= size_freed;
        m_stats.num_frees++;
    }

    MallocStats stats() const { return m_stats; };

private:
    MemoryBounds& m_mem_bounds;
    SpaceManager m_space_manager;
    MallocStats m_stats;
};
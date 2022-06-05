#include "malloc.hpp"
#include "twomath.hpp"
#include "units.hpp"

#include <new>

namespace pine {

void* FreeList::user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset)
{
    return reinterpret_cast<u8*>(node_ptr + 1) + offset;
}

FreeList::SizeNode* FreeList::construct_node(size_t region_size, void* node_location)
{
    return new (node_location) SizeNode(SizeData(region_size - sizeof(SizeNode)));
}

FreeList::SizeNode* FreeList::try_pick_free_node(size_t min_size)
{
    for (auto* node_ptr : m_free_list)
        if (min_size <= node_ptr->contents().reserved_size())
            return node_ptr;

    return nullptr;
}

Pair<void*, AllocationStats> FreeList::try_reserve(size_t requested_size)
{
    auto* node_ptr = try_pick_free_node(requested_size);
    if (!node_ptr)
        return { nullptr, 0 };

    auto& free_size_data = node_ptr->contents();
    free_size_data.reserve(requested_size);

    size_t aligned_remaining_size = align_down_two(free_size_data.remaining_size(), Alignment);
    if (aligned_remaining_size > allocation_size(Alignment)) {
        // split and assign free memory on the right side to a new node
        free_size_data.shrink_by(aligned_remaining_size);

        auto* remaining_location = user_addr_from_node_ptr(node_ptr, free_size_data.reserved_size());
        auto* new_node_ptr = construct_node(aligned_remaining_size, remaining_location);
        m_free_list.append(new_node_ptr);
    }

    m_free_list.detach(node_ptr);

    AllocationStats alloc_stats {
        free_size_data.requested_size(),
        sizeof(*node_ptr) + free_size_data.reserved_size()
    };
    return { user_addr_from_node_ptr(node_ptr), alloc_stats };
}

void FreeList::add(void* new_location, size_t new_size)
{
    auto* new_node_ptr = construct_node(new_size, new_location);
    m_free_list.append(new_node_ptr);
}

FreeList::SizeNode* FreeList::node_ptr_from_user_addr(void* addr)
{
    return static_cast<SizeNode*>(addr) - 1;
}

bool FreeList::nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr)
{
    auto end_of_left_region = reinterpret_cast<u8*>(user_addr_from_node_ptr(left_node_ptr)) + left_node_ptr->contents().reserved_size();
    return end_of_left_region == reinterpret_cast<u8*>(right_node_ptr);
}

Pair<FreeList::SizeNode*, FreeList::SizeNode*> FreeList::try_find_neighboring_memory_nodes(SizeNode* node_ptr)
{
    SizeNode* left_node_ptr = nullptr;
    SizeNode* right_node_ptr = nullptr;
    for (auto* free_node_ptr : m_free_list) {
        if (!left_node_ptr && free_node_ptr < node_ptr && nodes_are_contiguous_in_memory(free_node_ptr, node_ptr)) {
            left_node_ptr = free_node_ptr;
        } else if (!right_node_ptr && free_node_ptr > node_ptr && nodes_are_contiguous_in_memory(node_ptr, free_node_ptr)) {
            right_node_ptr = free_node_ptr;
        }
        if (left_node_ptr && right_node_ptr)
            break;
    }

    return { left_node_ptr, right_node_ptr };
}

void FreeList::adopt_right_node_size(SizeNode* left_node_ptr, SizeNode* right_node_ptr)
{
    auto& left_size_data = left_node_ptr->contents();
    size_t left_size = left_size_data.reserved_size();
    left_size += sizeof(SizeNode) + right_node_ptr->contents().reserved_size();
    left_size_data.grow_by(left_size);
}

AllocationStats FreeList::release(void* ptr)
{
    // FIXME: We should assert that the pointer belongs to us and is aligned:
    auto* node_ptr = node_ptr_from_user_addr(ptr);
    auto& freed_size_data = node_ptr->contents();
    size_t size_freed = freed_size_data.reserved_size();
    size_t requested_size = freed_size_data.requested_size();

    bool was_adopted = false;
    auto [left_node_ptr, right_node_ptr] = try_find_neighboring_memory_nodes(node_ptr);
    if (left_node_ptr) {
        adopt_right_node_size(left_node_ptr, node_ptr);
        size_freed += sizeof(SizeNode);

        node_ptr->~SizeNode();
        node_ptr = left_node_ptr; // can double adopt if right is next to us
        was_adopted = true;
    }
    if (right_node_ptr) {
        adopt_right_node_size(node_ptr, right_node_ptr);
        size_freed += sizeof(SizeNode);

        m_free_list.detach(right_node_ptr); // no longer free memory; belongs to node
        right_node_ptr->~SizeNode();
    }

    if (!was_adopted)
        m_free_list.append(node_ptr);

    return { requested_size, size_freed };
}

Pair<void*, size_t> FixedAllocation::allocate(size_t)
{
    if (!m_has_memory)
        return { nullptr, 0 };

    m_has_memory = false;
    return { reinterpret_cast<void*>(m_start), m_size };
}

void FixedAllocation::free(void*)
{
    m_has_memory = true;
}

bool FixedAllocation::in_bounds(void* ptr) const
{
    auto ptr_data = reinterpret_cast<PtrData>(ptr);
    return ptr_data >= m_start && ptr_data <= m_start + m_size;
}

Pair<void*, AllocationStats> HighWatermarkManager::try_reserve(size_t requested_size)
{
    if (m_watermark + requested_size > m_end)
        return { nullptr, { 0, 0 } };

    void* ptr = m_watermark;
    m_watermark += requested_size;
    return { ptr, requested_size };
}

void HighWatermarkManager::add(void* ptr, size_t size)
{
    m_start = reinterpret_cast<u8*>(ptr);
    m_watermark = m_start;
    m_end = m_start + size;
}

}

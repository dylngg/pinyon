#include "malloc.hpp"

void* FreeList::user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset)
{
    return reinterpret_cast<void*>(reinterpret_cast<PtrData>(node_ptr + 1) + offset);
}

FreeList::SizeNode* FreeList::construct_node(size_t region_size, void* node_location)
{
    return new (node_location) SizeNode(SizeData(region_size - sizeof(SizeNode)));
}

Pair<void*, AllocationStats> FreeList::try_find_memory(size_t requested_size)
{
    for (auto* node_ptr : m_free_list) {
        auto& free_size_data = node_ptr->contents();
        if (requested_size > free_size_data.reserved_size())
            continue;

        free_size_data.reserve(requested_size);
        size_t aligned_remaining_size = align_down_two(free_size_data.remaining_size(), ALIGNMENT_SIZE);
        if (aligned_remaining_size > min_allocation_size()) {
            free_size_data.shrink_by(aligned_remaining_size);

            auto* remaining_location = user_addr_from_node_ptr(node_ptr, free_size_data.reserved_size());
            auto* new_node_ptr = construct_node(aligned_remaining_size, remaining_location);
            m_free_list.append_node(new_node_ptr);
        }

        m_free_list.detach_node(node_ptr);
        AllocationStats alloc_stats {
            free_size_data.requested_size(),
            sizeof(SizeNode) + free_size_data.reserved_size()
        };
        return { user_addr_from_node_ptr(node_ptr), alloc_stats };
    }

    return { nullptr, {} };
}

void FreeList::add_memory(void* new_location, size_t new_size)
{
    auto* new_node_ptr = construct_node(new_size, new_location);
    m_free_list.append_node(new_node_ptr);
}

FreeList::SizeNode* FreeList::node_ptr_from_user_addr(void* addr)
{
    return reinterpret_cast<SizeNode*>(addr) - 1;
}

bool FreeList::nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr)
{
    auto end_of_left_region = reinterpret_cast<PtrData>(user_addr_from_node_ptr(left_node_ptr)) + left_node_ptr->contents().reserved_size();
    return end_of_left_region == reinterpret_cast<PtrData>(right_node_ptr);
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

AllocationStats FreeList::free_memory(void* ptr)
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

        m_free_list.detach_node(right_node_ptr); // no longer free memory; belongs to node
        right_node_ptr->~SizeNode();
    }

    if (!was_adopted)
        m_free_list.append_node(node_ptr);

    return { requested_size, size_freed };
}

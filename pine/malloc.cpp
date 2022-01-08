#include "malloc.hpp"

using SizeNode = LinkedList<size_t>::Node;

void* FreeList::user_addr_from_node_ptr(SizeNode* node_ptr, size_t offset)
{
    return reinterpret_cast<void*>(reinterpret_cast<PtrData>(node_ptr + 1) + offset);
}

SizeNode* FreeList::construct_node(size_t region_size, void* node_location)
{
    return new (node_location) SizeNode(region_size - sizeof(SizeNode));
}

Pair<void*, size_t> FreeList::try_find_memory(size_t requested_size)
{
    requested_size = align_up_two(requested_size, MIN_FREE_LIST_SIZE);
    constexpr size_t min_size = sizeof(SizeNode) + MIN_FREE_LIST_SIZE;

    for (auto* node_ptr : m_free_list) {
        size_t& available_size = node_ptr->contents();
        if (available_size < requested_size)
            continue;

        if (available_size - requested_size >= min_size) {
            size_t remaining_split_size = available_size - requested_size;
            available_size = requested_size;
            auto* new_node_ptr = construct_node(remaining_split_size, user_addr_from_node_ptr(node_ptr, requested_size));
            m_free_list.append_node(new_node_ptr);
        }

        m_free_list.detach_node(node_ptr);
        return { user_addr_from_node_ptr(node_ptr), sizeof(SizeNode) + node_ptr->contents() };
    }

    return { nullptr, 0 };
}

void FreeList::add_memory(void* new_location, size_t new_size)
{
    auto* new_node_ptr = construct_node(new_size, new_location);
    m_free_list.append_node(new_node_ptr);
}

SizeNode* FreeList::node_ptr_from_user_addr(void* addr)
{
    return reinterpret_cast<SizeNode*>(addr) - 1;
}

bool FreeList::nodes_are_contiguous_in_memory(SizeNode* left_node_ptr, SizeNode* right_node_ptr)
{
    auto end_of_left_region = reinterpret_cast<PtrData>(user_addr_from_node_ptr(left_node_ptr)) + left_node_ptr->contents();
    return end_of_left_region == reinterpret_cast<PtrData>(right_node_ptr);
}

Pair<SizeNode*, SizeNode*> FreeList::try_find_neighboring_memory_nodes(SizeNode* node_ptr)
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
    size_t& left_size = left_node_ptr->contents();
    left_size += sizeof(SizeNode) + right_node_ptr->contents();
}

size_t FreeList::free_memory(void* ptr)
{
    // FIXME: We should assert that the pointer belongs to us and is aligned:
    auto* node_ptr = node_ptr_from_user_addr(ptr);
    size_t size_freed = sizeof(SizeNode) + node_ptr->contents();

    bool was_adopted = false;
    auto [left_node_ptr, right_node_ptr] = try_find_neighboring_memory_nodes(node_ptr);
    if (left_node_ptr) {
        adopt_right_node_size(left_node_ptr, node_ptr);

        node_ptr->~SizeNode();
        node_ptr = left_node_ptr; // can double adopt if right is next to us
        was_adopted = true;
    }
    if (right_node_ptr) {
        adopt_right_node_size(node_ptr, right_node_ptr);

        m_free_list.detach_node(right_node_ptr); // no longer free memory; belongs to node
        right_node_ptr->~SizeNode();
    }

    if (!was_adopted)
        m_free_list.append_node(node_ptr);

    return size_freed;
}

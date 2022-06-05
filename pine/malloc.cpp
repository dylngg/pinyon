#include "malloc.hpp"
#include "twomath.hpp"
#include "units.hpp"

#include <new>

namespace pine {

void* FreeList::to_user_ptr(HeaderNode* node_ptr, size_t offset)
{
    return reinterpret_cast<u8*>(node_ptr + 1) + offset;
}

FreeList::HeaderNode* FreeList::construct_node(size_t region_size, void* node_location)
{
    return new (node_location) HeaderNode(Header{region_size - sizeof(HeaderNode)});
}

FreeList::HeaderNode* FreeList::find_first_free_node(size_t min_size)
{
    for (auto* node_ptr : m_free_list)
        if (min_size <= node_ptr->contents().size)
            return node_ptr;

    return nullptr;
}

Allocation FreeList::try_reserve(size_t requested_size)
{
    auto* node_ptr = find_first_free_node(requested_size);
    if (!node_ptr)
        return { nullptr, {} };

    size_t& alloc_size = node_ptr->contents().size;
    size_t remaining_size = align_down_two(alloc_size - requested_size, Alignment);

    if (remaining_size > min_allocation_size(Alignment)) {
        // split and assign free memory on the right side to a new node
        alloc_size -= remaining_size;

        void* new_node_location = to_user_ptr(node_ptr, alloc_size);
        m_free_list.append_node(construct_node(remaining_size, new_node_location));
    }

    m_free_list.remove(node_ptr);

    return { to_user_ptr(node_ptr), alloc_size };
}

void FreeList::add(void* new_location, size_t new_size)
{
    auto* new_node_ptr = construct_node(new_size, new_location);
    m_free_list.append_node(new_node_ptr);
}

FreeList::HeaderNode* FreeList::to_node_ptr(void* addr)
{
    return static_cast<HeaderNode*>(addr) - 1;
}

Pair<FreeList::HeaderNode*, FreeList::HeaderNode*> FreeList::try_find_neighboring_memory_nodes(HeaderNode* node_ptr)
{
    auto are_nodes_are_contiguous = [](HeaderNode* left, HeaderNode* right) {
        return reinterpret_cast<u8*>(to_user_ptr(left, left->contents().size)) == reinterpret_cast<u8*>(right);
    };

    HeaderNode* left_node_ptr = nullptr;
    HeaderNode* right_node_ptr = nullptr;
    for (auto* free_node_ptr : m_free_list) {
        if (!left_node_ptr && free_node_ptr < node_ptr && are_nodes_are_contiguous(free_node_ptr, node_ptr)) {
            left_node_ptr = free_node_ptr;
        } else if (!right_node_ptr && free_node_ptr > node_ptr && are_nodes_are_contiguous(node_ptr, free_node_ptr)) {
            right_node_ptr = free_node_ptr;
        }
        if (left_node_ptr && right_node_ptr)
            break;
    }

    return { left_node_ptr, right_node_ptr };
}

void FreeList::adopt_right_node_size(HeaderNode* left_node_ptr, HeaderNode* right_node_ptr)
{
    auto& left_size = left_node_ptr->contents().size;
    left_size += right_node_ptr->contents().size;
}

size_t FreeList::release(void* ptr)
{
    // FIXME: We should assert that the pointer belongs to us and is aligned:
    auto* node_ptr = to_node_ptr(ptr);
    size_t size_freed = node_ptr->contents().size;

    bool was_adopted = false;
    auto [left_node_ptr, right_node_ptr] = try_find_neighboring_memory_nodes(node_ptr);
    if (left_node_ptr) {
        adopt_right_node_size(left_node_ptr, node_ptr);

        node_ptr->~HeaderNode();
        node_ptr = left_node_ptr; // can double adopt if right is next to us
        was_adopted = true;
    }
    if (right_node_ptr) {
        adopt_right_node_size(node_ptr, right_node_ptr);

        m_free_list.remove(right_node_ptr); // no longer free memory; belongs to node
        right_node_ptr->~HeaderNode();
    }

    if (!was_adopted)
        m_free_list.append_node(node_ptr);

    return size_freed;
}

Pair<void*, size_t> FixedAllocation::allocate(size_t)
{
    if (!m_has_memory)
        return { nullptr, 0 };

    m_has_memory = false;
    return { reinterpret_cast<void*>(m_start), m_size };
}

size_t FixedAllocation::free(void*)
{
    m_has_memory = true;
    return m_size;
}

bool FixedAllocation::in_bounds(void* ptr) const
{
    auto ptr_data = reinterpret_cast<PtrData>(ptr);
    return ptr_data >= m_start && ptr_data <= m_start + m_size;
}

Allocation HighWatermarkManager::try_reserve(size_t requested_size)
{
    requested_size = align_up_two(requested_size, Alignment);
    if (m_watermark + requested_size > m_end)
        return {};

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

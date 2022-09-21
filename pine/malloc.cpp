#include "algorithm.hpp"
#include "malloc.hpp"

#include <new>

namespace pine {

void* FreeList::to_user_ptr(HeaderNode* node_ptr, size_t offset)
{
    return reinterpret_cast<u8*>(node_ptr + 1) + offset;
}

FreeList::HeaderNode* FreeList::construct_node(size_t region_size, void* node_location)
{
    auto header = Header{region_size - sizeof(HeaderNode), 0};
    return new (node_location) HeaderNode(header);
}

Allocation FreeList::try_reserve(size_t requested_size)
{
    auto it = find_if(m_free_list.begin(), m_free_list.end(), [&](const auto* node) {
        return node->contents().size >= requested_size;
    });
    if (it == m_free_list.end())
        return {};

    auto* node_ptr = *it;
    size_t& alloc_size = node_ptr->contents().size;
    size_t remaining_size = align_down_two(alloc_size - requested_size, Alignment);

    if (remaining_size > min_allocation_size(Alignment)) {
        // split and assign free memory on the right side to a new node
        alloc_size -= remaining_size;

        void* new_node_location = to_user_ptr(node_ptr, alloc_size);
        auto* new_node_ptr = construct_node(remaining_size, new_node_location);
        m_free_list.append(*new_node_ptr);
    }

    m_free_list.remove(node_ptr);

    return { to_user_ptr(node_ptr), alloc_size };
}

void FreeList::add(void* new_location, size_t new_size)
{
    auto* new_node_ptr = construct_node(new_size, new_location);
    m_free_list.append(*new_node_ptr);
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
        m_free_list.append(*node_ptr);

    return size_freed;
}

Allocation FixedAllocation::allocate(size_t amount)
{
    if (!m_has_memory || amount > m_size)
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

void PageBroker::init(PageRegion allocating_range, PageRegion scratch_pages)
{
    m_curr_runway = scratch_pages.size() - PageAllocatorBackend::initial_cost();
    m_page_allocator.init(allocating_range, scratch_pages);
}

bool PageBroker::allocate_required_runway()
{
    static_assert(c_required_runway < PageSize, "Cannot handle runway greater than a page in size");

    BrokeredAllocation brokered_alloc = m_page_allocator.allocate(1);
    if (!brokered_alloc)
        return false;

    m_curr_runway += brokered_alloc.size;
    m_curr_runway -= brokered_alloc.overhead_used;
    return true;
}

Allocation PageBroker::reserve_region(PageRegion region)
{
    if (m_curr_runway < c_required_runway && !allocate_required_runway())
        return {};

    auto broker_allocation = m_page_allocator.reserve_region(region);
    m_curr_runway -= broker_allocation.overhead_used;
    return broker_allocation.as_allocation();
}

Allocation PageBroker::allocate(unsigned num_pages, PageAlignmentLevel page_alignment)
{
    if (m_curr_runway < c_required_runway && !allocate_required_runway())
        return {};

    auto broker_allocation = m_page_allocator.allocate(num_pages, page_alignment);
    m_curr_runway -= broker_allocation.overhead_used;
    return broker_allocation.as_allocation();
}

Allocation PageBroker::allocate_huge_page(unsigned num_huge_pages)
{
    constexpr auto pages_in_huge_pages = HugePageSize / PageSize;
    return allocate(num_huge_pages * pages_in_huge_pages, PageAlignmentLevel::HugePage);
}

BrokeredAllocation PageAllocatorBackend::allocate(unsigned num_pages, PageAlignmentLevel page_alignment)
{
    auto* node = find_free_pages(num_pages, page_alignment);
    if (!node)
        return {};

    auto [region, num_splits] = remove_and_trim_pages(node, num_pages);
    return { region.ptr(), region.size(), num_splits * sizeof(Node) };
}

void PageBroker::free(Allocation allocation)
{
    m_page_allocator.free(allocation);
}

void PageAllocatorBackend::init(PageRegion allocating_range, PageRegion scratch_pages)
{
    add(scratch_pages);
    free_region(allocating_range);
}

BrokeredAllocation PageAllocatorBackend::reserve_region(PageRegion region)
{
    auto* node = find_free_region(region);
    if (!node)
        return {};

    auto [final_region, num_splits] = remove_and_trim_region(node, region);
    return { final_region.ptr(), final_region.size(), num_splits * sizeof(Node) };
}

void PageAllocatorBackend::add(PageRegion pages)
{
    m_node_allocator.add(pages.ptr(), pages.size());
}

AllocationCost PageAllocatorBackend::free_region(PageRegion region)
{
    auto [depth, node] = create_node(region);
    if (!node)
        return 0;  // ASSERT false? this should be guaranteed by the broker

    insert_node(depth, node);
    return FreeList::overhead();
}

void PageAllocatorBackend::free(Allocation allocation)
{
    auto region = PageRegion::from_ptr(allocation.ptr, allocation.size);
    free_region(region);
}

ManualLinkedList<PageRegion>::Node* PageAllocatorBackend::find_free_pages(unsigned num_pages, PageAlignmentLevel page_alignment)
{
    unsigned depth = depth_from_page_length(num_pages);

    while (depth != max_depth) {
        auto begin = m_free_lists[depth].begin();
        auto end = m_free_lists[depth].end();
        auto it = find_if(begin, end, [&](const Node* node) {
            return node->contents().fits(num_pages)
                && node->contents().aligned_to(static_cast<unsigned>(page_alignment));
        });
        if (it != end)
            return *it;

        ++depth;
    }

    return nullptr;
}

ManualLinkedList<PageRegion>::Node* PageAllocatorBackend::find_free_region(PageRegion region)
{
    unsigned depth = depth_from_page_length(region.length);

    while (depth != max_depth) {
        auto begin = m_free_lists[depth].begin();
        auto end = m_free_lists[depth].end();
        // FIXME: Use lower_bound, but bail if not less than
        auto it = find_if(begin, end, [&](const auto& node) { return node->contents() < region; });
        if (it != end)
            return *it;

        ++depth;
    }

    return nullptr;
}

Pair<PageRegion, AllocationCost> PageAllocatorBackend::trim_aligned_region(PageRegion curr_region, unsigned curr_depth, unsigned end_depth)
{
    while (curr_depth != end_depth) {
        auto [reserved_region, remainder_region] = curr_region.halve();
        --curr_depth;

        auto [_, node] = create_node(remainder_region);
        if (!node)
            break;

        insert_node(curr_depth, node);
        curr_region = reserved_region;
    }

    return { curr_region, (end_depth - curr_depth) * FreeList::overhead() };
}

Pair<PageRegion, AllocationCost> PageAllocatorBackend::remove_and_trim_pages(ManualLinkedList<PageRegion>::Node* node, unsigned min_pages)
{
    auto curr_region = node->contents();
    auto curr_depth = depth_from_page_length(curr_region.length);

    remove_node(curr_depth, node);

    auto num_pages_aligned = align_up_to_power(min_pages);
    auto min_depth = depth_from_page_length(num_pages_aligned);

    return trim_aligned_region(curr_region, curr_depth, min_depth);
}

Pair<PageRegion, AllocationCost> PageAllocatorBackend::remove_and_trim_region(ManualLinkedList<PageRegion>::Node* node, PageRegion min_region)
{
    auto curr_region = node->contents();
    auto curr_depth = depth_from_page_length(curr_region.length);

    remove_node(curr_depth, node);

    // min.ptr >= curr.ptr, curr.end_ptr <= min.end_ptr
    //     |  min  |
    // |     curr    |
    auto left_trim = PageRegion {curr_region.offset, min_region.offset - curr_region.offset };
    auto right_trim = PageRegion {min_region.end_offset(), curr_region.end_offset() - min_region.end_offset() };

    AllocationCost total_distance = 0;
    if (left_trim)
        total_distance += free_region(left_trim);
    if (right_trim)
        total_distance += free_region(right_trim);

    return {PageRegion {left_trim.end_offset(), right_trim.offset }, total_distance };
}

Pair<unsigned, PageAllocatorBackend::Node*> PageAllocatorBackend::create_node(PageRegion region)
{
    auto allocation = m_node_allocator.try_reserve(sizeof(Node));
    if (!allocation.ptr)
        return { 0, nullptr };

    unsigned depth = depth_from_page_length(region.length);
    return { depth, new (static_cast<Node*>(allocation.ptr)) Node(region) };
}

void PageAllocatorBackend::remove_node(unsigned depth, ManualLinkedList<PageRegion>::Node* node)
{
    m_free_lists[depth].remove(node);
    m_node_allocator.release(static_cast<void*>(node));
}

void PageAllocatorBackend::insert_node(unsigned depth, Node* node)
{
    auto begin = m_free_lists[depth].begin();
    auto end = m_free_lists[depth].end();
    auto insertion_pt = lower_bound(begin, end, node->contents(), ManualLinkedList<PageRegion>::Greater{});
    m_free_lists[depth].insert_after(*insertion_pt, *node);
}

void print_with(Printer& printer, const PageBroker& page_broker)
{
    print_with(printer, page_broker.m_page_allocator);
    print_with(printer, " [runway=");
    print_with(printer, page_broker.m_curr_runway);
    print_with(printer, " bytes]");
}

void print_with(Printer& printer, const PageAllocatorBackend& alloc)
{
    print_with(printer, "PageAllocator {\n");
    for (const auto& region_list : alloc.m_free_lists) {
        print_with(printer, "\t");
        print_with(printer, region_list);
        print_with(printer, ",\n");
    }

    print_with(printer, "}");
}

}

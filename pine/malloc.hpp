#pragma once
#include "c_builtins.hpp"
#include "linked_list.hpp"
#include "maybe.hpp"
#include "twomath.hpp"
#include "types.hpp"
#include "page.hpp"
#include "print.hpp"
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

    operator bool() const { return size != 0; }
};

class FreeList {
    // Use padding in struct to ensure alignment.
    // e.g. ManualLinkedList<size_t>::Node on ARM32 is 12 bytes, which is not 8
    //      byte aligned
    struct Header {
        size_t size;
        size_t _padding;
    };
    using HeaderNode = ManualLinkedList<Header>::Node;
    static_assert(sizeof(HeaderNode) % Alignment == 0);
    static_assert(sizeof(Header) % Alignment == 0);

public:
    FreeList() = default;

    static size_t preferred_size(size_t requested_size)
    {
        return align_up_two(min_allocation_size(requested_size), PageSize);
    }
    static constexpr size_t overhead()
    {
        return sizeof(HeaderNode);
    }

    Allocation try_reserve(size_t);
    void add(void*, size_t);
    size_t release(void*);

private:
    constexpr static size_t min_allocation_size(size_t requested_size)
    {
        size_t alloc_size = align_up_two(requested_size, Alignment) + overhead();
        static_assert(is_aligned_two(sizeof(HeaderNode), Alignment), "Free list header size is not aligned!");
        return alloc_size;
    }
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

    Allocation allocate(size_t requested_size)
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

    size_t free(Allocation alloc)
    {
        if (!alloc)
            return 0; // FIXME: assert?!

        size_t size_freed = m_manager.release(alloc.ptr);
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

    Allocation allocate(size_t requested_size);
    size_t free(void*);
    bool in_bounds(void* ptr) const;

private:
    PtrData m_start;
    size_t m_size;
    bool m_has_memory;
};

using FixedHeapAllocator = MemoryAllocator<FixedAllocation, HighWatermarkManager>;

struct BrokeredAllocation {
    void* ptr;
    size_t size;
    size_t overhead_used;

    [[nodiscard]] Allocation as_allocation() const
    {
        return { ptr, size };
    }
    operator bool() const { return size != 0; }
};

using AllocationCost = PtrData;

enum class PageAlignmentLevel : unsigned {
    Page = 1,
    HugePage = HugePageSize / PageSize,
};


class PageAllocatorBackend {
public:
    PageAllocatorBackend() = default;
    void init(PageRegion allocating_range, PageRegion scratch_pages);

    [[nodiscard]] BrokeredAllocation reserve_region(PageRegion region);
    [[nodiscard]] BrokeredAllocation allocate(unsigned num_pages, PageAlignmentLevel page_alignment = PageAlignmentLevel::Page);
    void add(PageRegion);
    void free(Allocation);

    static constexpr size_t max_overhead()
    {
        return FreeList::overhead() * max_depth;
    }
    static constexpr size_t initial_cost()
    {
        return FreeList::overhead();
    }

    friend void print_with(Printer&, const PageAllocatorBackend&);

private:
    static unsigned depth_from_page_length(unsigned num_pages)
    {
        if (num_pages == 0)
            return 0;

        static_assert(bit_width(1u) == 1);
        return bit_width(num_pages) - 1;
    }

    using Node = ManualLinkedList<PageRegion>::Node;

    static constexpr unsigned max_depth = (sizeof(size_t) * CHAR_BIT) - bit_width(PageSize) + 2;

    Node* find_free_pages(unsigned num_pages, PageAlignmentLevel page_alignment);
    Node* find_free_region(PageRegion);
    Pair<PageRegion, AllocationCost> remove_and_trim_pages(Node* node, unsigned min_pages);
    Pair<PageRegion, AllocationCost> remove_and_trim_region(Node* node, PageRegion min_region);
    Pair<PageRegion, AllocationCost> trim_aligned_region(PageRegion curr_region, unsigned curr_depth, unsigned end_depth);
    AllocationCost free_region(PageRegion);
    Pair<unsigned, Node*> create_node(PageRegion);
    void remove_node(unsigned depth, Node* node);
    void insert_node(unsigned depth, Node* node);

    FreeList m_node_allocator {};  // FIXME: Replace with a more space-efficient bitfield based allocator (slab)
    ManualLinkedList<PageRegion> m_free_lists[max_depth] {};  // 0 is PageSize
};

/*
 * Ensures that the allocator has enough memory required for internal needs
 * by acting as an intermediary allocator. The broker may request more memory
 * from the allocator than necessary in order to do so.
 *
 * The broker mai=ntains the invariant that the allocator can always free or
 * allocate memory unless there is not enough memory in the system to do so.
 * (the allocation or free will not fail because there is not enough scratch
 *  space for metadata)
 */
class PageBroker {
public:
    PageBroker() = default;
    void init(PageRegion allocating_range, PageRegion scratch_pages)
    {
        m_curr_runway = scratch_pages.size() - PageAllocatorBackend::initial_cost();
        m_page_allocator.init(allocating_range, scratch_pages);
    }

    template <typename... Args>
    Allocation allocate(Args... args)
    {
        if (m_curr_runway < c_required_runway && !allocate_required_runway())
            return {};

        auto broker_allocation = m_page_allocator.allocate(pine::forward<Args>(args)...);
        m_curr_runway -= broker_allocation.overhead_used;
        return broker_allocation.as_allocation();
    }
    template <typename... Args>
    Allocation reserve_region(Args... args)
    {
        if (m_curr_runway < c_required_runway && !allocate_required_runway())
            return {};

        auto broker_allocation = m_page_allocator.reserve_region(pine::forward<Args>(args)...);
        m_curr_runway -= broker_allocation.overhead_used;
        return broker_allocation.as_allocation();
    }
    void free(Allocation alloc)
    {
        m_page_allocator.free(alloc);
    }

    friend void print_with(Printer&, const PageBroker& page_broker);

private:
    [[nodiscard]] bool allocate_required_runway()
    {
        static_assert(c_required_runway < PageSize, "Cannot handle runway greater than a page in size");

        BrokeredAllocation brokered_alloc = m_page_allocator.allocate(1, PageAlignmentLevel::Page);
        if (!brokered_alloc)
            return false;

        m_curr_runway += brokered_alloc.size;
        m_curr_runway -= brokered_alloc.overhead_used;
        return true;
    }

    static constexpr size_t c_required_runway = align_up_to_power(PageAllocatorBackend::max_overhead());

    size_t m_curr_runway = 0;
    PageAllocatorBackend m_page_allocator {};
};

using PageAllocator = PageBroker;

}

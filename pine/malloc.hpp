#pragma once
#include "array.hpp"
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

class IntrusiveFreeList {
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
    IntrusiveFreeList() = default;

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

template <typename Value>
struct SlabAllocator {
    static constexpr unsigned c_bits_per_bitmap = (PageSize * CHAR_BIT) - (PageSize % sizeof(Value));
    static constexpr unsigned c_slabs_per_page = PageSize / sizeof(Value);

public:
    static size_t preferred_size(size_t requested_size)
    {
        return align_up_two(requested_size, PageSize) + PageSize;
    }

    pine::Allocation try_reserve(size_t size = sizeof(Value))
    {
        if (size != sizeof(Value))  // ASSERT?
            return {};

        auto [found, bitmap_page_index, index_in_bitmap_page] = try_find_free_bit();
        if (!found)
            return {};

        auto& bitmap = (*next(m_bitmaps.begin(), bitmap_page_index))->contents();
        bitmap.set_bit(index_in_bitmap_page, true);  // FIXME: Create mark_bit method

        auto [slab_page_index, index_in_slab_page] = calculate_slab_indexes(bitmap_page_index, index_in_bitmap_page);
        auto slab_page = (*next(m_slab_pages.begin(), slab_page_index))->contents();

        auto* slab_page_ptr = reinterpret_cast<Value*>(slab_page);
        return { slab_page_ptr + index_in_slab_page, sizeof(Value) };
    }
    void add(void* ptr, size_t size)
    {
        constexpr unsigned c_chunk_size = max(align_up_two(sizeof(BitMapNode), PageSize), align_up_two(sizeof(SlabNode), PageSize));

        auto remove_page_from_variables = [&](void*& _ptr, size_t& _size) {
            _size -= c_chunk_size;
            _ptr = reinterpret_cast<void*>(reinterpret_cast<PtrData>(ptr) + c_chunk_size);
        };

        while (size >= c_chunk_size) {
            if (m_bitmaps.length() * c_bits_per_bitmap - num_slabs() < c_slabs_per_page) {
                auto bitmap_node_alloc = m_free_pages.try_reserve(sizeof(BitMapNode));
                if (!bitmap_node_alloc) {
                    m_free_pages.add(ptr, c_chunk_size);
                    remove_page_from_variables(ptr, size);
                    bitmap_node_alloc = m_free_pages.try_reserve(sizeof(BitMapNode));
                    if (!bitmap_node_alloc)  // FIXME: ASSERT bitmap_node_alloc
                        break;
                }
                auto bitmap_node = new (bitmap_node_alloc.ptr) BitMapNode();
                m_bitmaps.append(*bitmap_node);
            }

            if (size < c_chunk_size)
                break;

            auto slab_node_alloc = m_free_pages.try_reserve(sizeof(SlabNode));
            if (!slab_node_alloc) {
                m_free_pages.add(ptr, c_chunk_size);
                remove_page_from_variables(ptr, size);
                slab_node_alloc = m_free_pages.try_reserve(sizeof(SlabNode));
                if (!slab_node_alloc)  // FIXME: ASSERT slab_node_alloc
                    break;
            }
            auto slab_node = new (slab_node_alloc.ptr) SlabNode(reinterpret_cast<PtrData>(ptr));
            remove_page_from_variables(ptr, size);
            m_slab_pages.append(*slab_node);
        }

    }
    size_t release(void* ptr)
    {
        PtrData slab_ptr_data = reinterpret_cast<PtrData>(ptr);

        auto [slab_page_index, index_in_slab_page] = find_associated_slab_page(slab_ptr_data);
        auto [bitmap_page_index, index_in_bitmap_page] = calculate_bitmap_indexes(slab_page_index, index_in_slab_page);

        auto& bitmap = (*next(m_bitmaps.begin(), bitmap_page_index))->contents();
        bitmap.set_bit(index_in_bitmap_page, true);
        return sizeof(Value);
    }

private:
    size_t num_slabs() const { return m_slab_pages.length() * c_slabs_per_page; }
    Triple<bool, size_t, size_t> try_find_free_bit()
    {
       size_t page_index = 0;
        for (auto& bitmap_node : m_bitmaps) {
            SlabBitMap &bitmap = bitmap_node->contents();

            size_t i = 0;
            for (; i < c_bits_per_bitmap && calculate_index_from_bitmap_indexes(page_index, i) < num_slabs(); i++)
                if (!bitmap.bit(i))
                    return {true, page_index, i };

            // Try next bitmap
            ++page_index;
        }
        return { false, 0, 0 };
    }
    Pair<size_t, size_t> find_associated_slab_page(PtrData slab)
    {
        auto slab_page = align_down_two(slab, PageSize);

        size_t slab_page_index = 0;
        for (auto& slab_page_node : m_slab_pages) {
            if (slab_page_node->contents() == slab_page)
                break;

            ++slab_page_index;
        }

        return { slab_page_index, (slab - slab_page) / sizeof(Value) };
    }
    size_t calculate_index_from_bitmap_indexes(size_t bitmap_page_index, size_t index_in_bitmap_page) const
    {
        return (bitmap_page_index * c_bits_per_bitmap) + index_in_bitmap_page;
    }
    size_t calculate_index_from_slab_indexes(size_t slab_page_index, size_t index_in_slab_page) const
    {
        return (slab_page_index * c_slabs_per_page) + index_in_slab_page;
    }
    Pair<size_t, size_t> calculate_slab_indexes(size_t bitmap_page_index, size_t index_in_bitmap_page) const
    {
        size_t index = calculate_index_from_bitmap_indexes(bitmap_page_index, index_in_bitmap_page);
        return { index / c_slabs_per_page, index % c_slabs_per_page };
    }
    Pair<size_t, size_t> calculate_bitmap_indexes(size_t slab_page_index, size_t index_in_slab_page) const
    {
        size_t index = calculate_index_from_slab_indexes(slab_page_index, index_in_slab_page);
        return { index / c_bits_per_bitmap, index % c_bits_per_bitmap };
    }

    static_assert(PageSize >= sizeof(Value));

    using SlabBitMap = BitMap<c_bits_per_bitmap>;
    using BitMapNode = typename ManualLinkedList<SlabBitMap>::Node;
    using SlabNode = typename ManualLinkedList<PtrData>::Node;

    ManualLinkedList<SlabBitMap> m_bitmaps {};
    ManualLinkedList<PtrData> m_slab_pages {};
    IntrusiveFreeList m_free_pages {};
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
    [[nodiscard]] BrokeredAllocation allocate(size_t num_pages, PageAlignmentLevel page_alignment = PageAlignmentLevel::Page);
    void add(PageRegion);
    void free(Allocation);

    static constexpr size_t max_overhead()
    {
        return IntrusiveFreeList::overhead() * max_depth;
    }
    static constexpr size_t initial_cost()
    {
        return IntrusiveFreeList::overhead();
    }

    friend void print_with(Printer&, const PageAllocatorBackend&);

private:
    static unsigned depth_from_page_length(size_t num_pages)
    {
        if (num_pages == 0)
            return 0;

        static_assert(bit_width(1u) == 1);
        return static_cast<unsigned>(bit_width(num_pages) - 1);
    }

    using Node = ManualLinkedList<PageRegion>::Node;

    static constexpr unsigned max_depth = (sizeof(size_t) * CHAR_BIT) - bit_width(PageSize) + 2;

    Node* find_free_pages(size_t num_pages, PageAlignmentLevel page_alignment);
    Node* find_free_region(PageRegion);
    Pair<PageRegion, AllocationCost> remove_and_trim_pages(Node* node, size_t min_pages);
    Pair<PageRegion, AllocationCost> remove_and_trim_region(Node* node, PageRegion min_region);
    Pair<PageRegion, AllocationCost> trim_aligned_region(PageRegion curr_region, unsigned curr_depth, unsigned end_depth);
    AllocationCost free_region(PageRegion);
    Pair<unsigned, Node*> create_node(PageRegion);
    void remove_node(unsigned depth, Node* node);
    void insert_node(unsigned depth, Node* node);

    SlabAllocator<Node> m_node_allocator {};
    Array<ManualLinkedList<PageRegion>, max_depth> m_free_lists {};  // 0 is PageSize
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
        BrokeredAllocation brokered_alloc = m_page_allocator.allocate(c_required_runway_in_pages, PageAlignmentLevel::Page);
        if (!brokered_alloc)
            return false;

        m_curr_runway += brokered_alloc.size;
        m_curr_runway -= brokered_alloc.overhead_used;
        return true;
    }

    static constexpr size_t c_required_runway = align_up_to_power(PageAllocatorBackend::max_overhead());
    static constexpr size_t c_required_runway_in_pages = align_up_two(c_required_runway, PageSize);

    size_t m_curr_runway = 0;
    PageAllocatorBackend m_page_allocator {};
};

using PageAllocator = PageBroker;

}

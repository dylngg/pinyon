#pragma once

#include <pine/array.hpp>
#include <pine/malloc.hpp>
#include <pine/print.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>

#define DEVICES_START 0x3F000000
#define DEVICES_END 0x40000000
#define HEAP_START 0x20000000
#define HEAP_END 0x24000000
#define MEMORY_END_PAGES 1048576  // 2^32 / PageSize

// We do conversions between bitfields and full types all the time around here,
// not worth the extra reinterpret_cast<>(*) everywhere...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

struct VirtualAddress {
    VirtualAddress(PtrData ptr)
        : m_ptr(reinterpret_cast<u8*>(ptr)) {};
    VirtualAddress(void* ptr)
        : m_ptr(reinterpret_cast<u8*>(ptr)) {};

    /* Note: Table entries are 4 bytes */
    static VirtualAddress from_l1_index(unsigned index) { return VirtualAddress(L1Tag{}, static_cast<u32>(index)); };
    int l1_index() const { return m_as.l1_table_index; }
    static VirtualAddress from_l2_index(unsigned index) { return VirtualAddress(L2Tag{}, static_cast<u32>(index)); };
    int l2_index() const { return m_as.l2_table_index; }

    void* ptr() const { return m_ptr; }

    operator void*() const { return ptr(); }

private:
    struct L1Tag {};
    struct L2Tag {};
    VirtualAddress(L1Tag, u32 index)
        : m_as(As{0, 0, index, 0}) {};
    VirtualAddress(L2Tag, u32 index)
        : m_as(As{0, index, 0, 0}) {};

    // See Figure B3-11 Small page address translation in ARMv7 Reference Manual
    struct As {
        u32 _ : 12;
        u32 l2_table_index : 8;
        u32 l1_table_index : 10;
        u32 __ : 2;
    };
    union {
        As m_as;
        u8* m_ptr;
    };
};

static_assert(sizeof(VirtualAddress) == sizeof(PtrData));

struct PhysicalAddress {
    explicit PhysicalAddress(PtrData ptr)
        : m_ptr(ptr) {};
    explicit PhysicalAddress(void* ptr)
        : m_ptr(reinterpret_cast<PtrData>(ptr)) {};

    static PhysicalAddress from_l1_base_addr(PtrData l1_base_addr)
    {
        PhysicalAddress addr {};
        addr.m_as_l1.base_addr = l1_base_addr;
        return addr;
    }
    static PhysicalAddress from_l2_base_addr(PtrData l2_base_addr)
    {
        PhysicalAddress addr {};
        addr.m_as_l2.base_addr = l2_base_addr;
        return addr;
    }

    PtrData ptr_data() const { return reinterpret_cast<PtrData>(m_ptr); }
    PtrData l1_base_addr() const { return m_as_l1.base_addr; }
    PtrData l2_base_addr() const { return m_as_l2.base_addr; }

    PhysicalAddress operator+(size_t amount) { return PhysicalAddress { m_ptr += amount }; };
    PhysicalAddress& operator+=(size_t amount)
    {
        m_ptr += amount;
        return *this;
    };

private:
    PhysicalAddress()
        : m_ptr(0) {};

    struct AsL1 {
        u32 _ : 20;
        PtrData base_addr : 12;
    };
    struct AsL2 {
        u32 _ : 12;
        PtrData base_addr : 20;
    };
    union {
        AsL1 m_as_l1;
        AsL2 m_as_l2;
        PtrData m_ptr;
    };
};

static_assert(sizeof(PhysicalAddress) == sizeof(PtrData));

namespace mmu {

enum class L1Type : u32 {
    Fault = 0,
    L2Ptr = 1,
    HugePage = 2,
    SuperSection = 3,
};

inline L1Type l1_type_from_bits(u32 bits)
{
    L1Type type;
    static_assert(sizeof(type) == sizeof(bits));
    memcpy (&type, &bits, sizeof(type));
    return type;
}

enum class L2Type : u32 {
    Fault = 0,
    LargePage = 1,
    Page = 2,
};

inline L2Type l2_type_from_bits(u32 bits)
{
    L2Type type;
    static_assert(sizeof(type) == sizeof(bits));
    memcpy (&type, &bits, sizeof(type));
    return type;
}

struct L2Fault {
    L2Fault()
        : type(L2Type::Fault)
        , _(0) {};

    L2Type type : 2;
    u32 _ : 30;
};

struct LargePage {
    L2Type type : 2;
    u32 b : 1;
    u32 c : 1;
    u32 ap : 2;
    u32 sbz : 3;
    u32 apx : 1;
    u32 s : 1;
    u32 nG : 1;
    u32 tex : 3;
    u32 xn : 1;
    u32 base_addr : 16;
};

struct Page {
    Page(PhysicalAddress addr)
        : type(L2Type::Page)
        , b(0)
        , c(0)
        , ap(0b11)
        , sbz(0)
        , apx(0)
        , s(0)
        , nG(0)
        , base_addr(addr.l1_base_addr()) {};

    L2Type type : 2;
    u32 b : 1;
    u32 c : 1;
    u32 ap : 2;
    u32 sbz : 3;
    u32 apx : 1;
    u32 s : 1;
    u32 nG : 1;
    u32 base_addr : 20;
};

union L2Entry {
    L2Entry()
        : _() {};

    static constexpr auto vm_size = 4 * KiB;

    L2Type type() const { return l2_type_from_bits(static_cast<u32>(_.type)); }

    L2Entry& operator=(const LargePage& large_page)
    {
        as_large_page = large_page;
        return *this;
    }
    L2Entry& operator=(const Page& page)
    {
        as_page = page;
        return *this;
    }

    L2Fault _;
    LargePage as_large_page;
    Page as_page;
};

struct L2Table {
    L2Table() = default;

    static constexpr auto num_entries = 256;

    L2Entry& retrieve_entry(VirtualAddress virt_addr) { return m_entries[virt_addr.l2_index()]; };

    friend void print_with(pine::Printer&, const L2Table&);

private:
    pine::Array<L2Entry, num_entries> m_entries;
};

struct L2Ptr {
    L2Ptr(PhysicalAddress addr)
        : type(L1Type::L2Ptr)
        , sbz(0)
        , domain(0)
        , p(0)
        , base_addr(addr.l2_base_addr()) {};

    L1Type type : 2;
    u32 sbz : 3;
    u32 domain : 4;
    u32 p : 1;
    u32 base_addr : 22;

    [[nodiscard]] L2Table* l2_table() const
    {
        return reinterpret_cast<L2Table*>(PhysicalAddress::from_l2_base_addr(base_addr).ptr_data());
    }

    PhysicalAddress physical_address() const { return PhysicalAddress::from_l2_base_addr(base_addr); }

    friend void print_with(pine::Printer&, const L2Ptr&);
};

struct L1Fault {
    L1Fault()
        : type(L1Type::Fault)
        , _(0) {};

    L1Type type : 2;
    u32 _ : 30;
};



struct HugePage {
    HugePage(PhysicalAddress addr)
        : type(L1Type::HugePage)
        , b(0)
        , c(0)
        , xn(0)
        , domain(0)
        , p(0)
        , ap(0b11)
        , tex(0)
        , apx(0)
        , s(0)
        , nG(0)
        , _(0)
        , sbz(0)
        , base_addr(addr.l1_base_addr()) {};

    PhysicalAddress physical_address() const { return PhysicalAddress::from_l1_base_addr(base_addr); }

    L1Type type : 2;
    u32 b : 1;
    u32 c : 1;
    u32 xn : 1;
    u32 domain : 4;
    u32 p : 1;
    u32 ap : 2;
    u32 tex : 3;
    u32 apx : 1;
    u32 s : 1;
    u32 nG : 1;
    u32 _ : 1;
    u32 sbz : 1;
    PtrData base_addr : 10;

    friend void print_with(pine::Printer&, const HugePage&);
};

struct SuperSection {
    L1Type type : 2;
    u32 b : 1;
    u32 c : 1;
    u32 xn : 1;
    u32 domain : 4;
    u32 p : 1;
    u32 ap : 2;
    u32 tex : 3;
    u32 apx : 1;
    u32 s : 1;
    u32 nG : 1;
    u32 _ : 1;
    u32 sbz : 5;
    u32 base_addr : 8;

    PhysicalAddress physical_address() const { return PhysicalAddress::from_l1_base_addr(base_addr); }

    friend void print_with(pine::Printer&, const SuperSection&);
};

union L1Entry {
    L1Entry()
        : _() {};

    static constexpr auto vm_size = MiB;

    L1Entry& operator=(const L2Ptr& ptr)
    {
        as_ptr = ptr;
        return *this;
    }
    L1Entry& operator=(const HugePage& huge_page)
    {
        as_huge_page = huge_page;
        return *this;
    }
    L1Entry& operator=(const SuperSection& super_huge_page)
    {
        as_super_huge_page = super_huge_page;
        return *this;
    }

    L1Type type() const { return l1_type_from_bits(static_cast<u32>(_.type)); }

    L1Fault _;
    L2Ptr as_ptr;
    HugePage as_huge_page;
    SuperSection as_super_huge_page;
};

struct L1Table {
    L1Table() = default;

    static constexpr auto num_entries = 4096;

    L1Entry& retrieve_entry(VirtualAddress virt_addr) { return m_entries[virt_addr.l1_index()]; };

    friend void print_with(pine::Printer&, const L1Table&);

private:
    pine::Array<L1Entry, num_entries> m_entries;
};

#pragma GCC diagnostic pop

extern "C" void init_page_tables(PtrData code_end);

void set_l1_table(L1Table& l1_addr);

L1Table& l1_table();

using PhysicalPageAllocator = pine::PageAllocator;
using VirtualPageAllocator = pine::PageAllocator;

PhysicalPageAllocator& physical_page_allocator();
VirtualPageAllocator& virtual_page_allocator();

class PageAllocator {
public:
    PageAllocator() = default;
    void init(L1Table&, PhysicalPageAllocator&, VirtualPageAllocator&, PageRegion scratch_region);

    enum class Backing {
        Mixed = 0,
        Identity = 1,
    };

    Pair<PageRegion, PageRegion> reserve_region(PageRegion, Backing = Backing::Mixed);
    Pair<HugePageRegion, HugePageRegion> reserve_huge_page_region(HugePageRegion , Backing = Backing::Mixed);
    Pair<PageRegion, PageRegion> allocate_pages(unsigned num_pages, pine::PageAlignmentLevel = pine::PageAlignmentLevel::Page, Backing = Backing::Mixed);
    void free(pine::Allocation);

private:
    Pair<pine::Allocation, pine::Allocation> try_reserve_page_unrecorded(unsigned num_pages, pine::PageAlignmentLevel, Backing);
    Pair<pine::Allocation, pine::Allocation> try_reserve_region_unrecorded(PageRegion, Backing);
    bool try_record_page_in_l1(PageRegion phys_region, PageRegion virt_region, void* l2_backing = nullptr);
    bool try_record_huge_page_in_l1(HugePageRegion phys_region, HugePageRegion virt_region);
    pine::Allocation try_reserve_l2_table_entry();

    friend void init_page_tables(PtrData);

    PhysicalPageAllocator* m_physical_page_allocator = nullptr;
    VirtualPageAllocator* m_virtual_page_allocator = nullptr;
    L1Table* m_l1_table = nullptr;
    pine::SlabAllocator<L2Table> m_l2_table_allocator;
    void* m_spare_free_l2_page;
};

}

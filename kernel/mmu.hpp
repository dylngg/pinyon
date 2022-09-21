#pragma once

#include <pine/malloc.hpp>
#include <pine/print.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>

#define DEVICES_START 0x3F000000
#define DEVICES_END 0x3FFFFFFF
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
    static VirtualAddress from_l1_index(int index) { return VirtualAddress(L1Tag{}, static_cast<u32>(index)); };
    int l1_index() const { return m_as.l1_table_index; }
    static VirtualAddress from_l2_index(int index) { return VirtualAddress(L2Tag{}, static_cast<u32>(index)); };
    int l2_index() const { return m_as.l2_table_index; }

    void* ptr() const { return m_ptr; }

    operator void*() const { return ptr(); }

private:
    struct L1Tag {};
    struct L2Tag {};
    VirtualAddress(L1Tag, u32 index)
        : m_as(As{0, 0, index}) {};
    VirtualAddress(L2Tag, u32 index)
        : m_as(As{0, index, 0}) {};

    struct As {
        u32 _ : 10;
        u32 l2_table_index : 10;
        u32 l1_table_index : 12;
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
    Section = 2,
    SuperSection = 3,
};

inline L1Type l1_type_from_bits(u32 bits)
{
    L1Type type;
    static_assert(sizeof(type) == sizeof(bits));
    memcpy (&type, &bits, sizeof(type));
    return type;
}

struct Fault {
    Fault()
        : type(L1Type::Fault)
        , _(0) {};

    L1Type type : 2;
    u32 _ : 30;
};

struct L2Ptr {
    L1Type type : 2;
    u32 sbz : 3;
    u32 domain : 4;
    u32 p : 1;
    u32 base_addr : 22;

    PhysicalAddress physical_address() const { return PhysicalAddress::from_l2_base_addr(base_addr); }

    friend void print_with(pine::Printer&, const L2Ptr&);
};

struct Section {
    Section(PhysicalAddress addr)
        : type(L1Type::Section)
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

    friend void print_with(pine::Printer&, const Section&);
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
    L1Entry& operator=(const Section& section)
    {
        as_section = section;
        return *this;
    }
    L1Entry& operator=(const SuperSection& super_section)
    {
        as_super_section = super_section;
        return *this;
    }

    L1Type type() const { return l1_type_from_bits(static_cast<u32>(as_ptr.type)); }

    Fault _;
    L2Ptr as_ptr;
    Section as_section;
    SuperSection as_super_section;
};

struct L1Table {
    L1Table() = default;

    static constexpr auto num_entries = 4096;

    void reserve_section(VirtualAddress, Section);

    template <typename... SArgs>
    void reserve_section(VirtualAddress vm_addr, SArgs&& ...args) { reserve_section(vm_addr, Section(pine::forward<SArgs>(args)...)); }
    void reserve_section_region(SectionRegion vm_region, SectionRegion phys_region)
    {
        auto phys_addr = reinterpret_cast<PtrData>(phys_region.ptr());
        for (auto addr = reinterpret_cast<PtrData>(vm_region.ptr()); addr < reinterpret_cast<PtrData>(vm_region.end_ptr()); addr += HugePageSize) {
            reserve_section(addr, PhysicalAddress(phys_addr));
            phys_addr += HugePageSize;
        }
    }
    void reserve_identity_section_region(SectionRegion region)
    {
        for (auto addr = reinterpret_cast<PtrData>(region.ptr()); addr < reinterpret_cast<PtrData>(region.end_ptr()); addr += HugePageSize)
            reserve_section(addr, PhysicalAddress(addr));
    }

    friend void print_with(pine::Printer&, const L1Table&);

private:
    L1Entry m_entries[num_entries];
};

enum class L2Type : u32 {
    Fault = 0,
    LargePage = 1,
    Page = 2,
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
    static constexpr auto vm_size = 4 * KiB;

    Fault _;
    LargePage large_page;
    Page page;
};

struct L2Table {
    static constexpr auto num_entries = 256;

    L2Entry m_entries[num_entries];
};

extern "C" void init_page_tables(PtrData code_end);

void set_l1_table(PhysicalAddress l1_addr);

PhysicalAddress l1_table();

}

using PhysicalPageAllocator = pine::PageAllocator;
using VirtualPageAllocator = pine::PageAllocator;

PhysicalPageAllocator& physical_page_allocator();
VirtualPageAllocator& virtual_page_allocator();

#pragma GCC diagnostic pop

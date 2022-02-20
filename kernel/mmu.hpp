#pragma once

#include <pine/types.hpp>
#include <pine/units.hpp>

#define DEVICES_START 0x3F000000
#define DEVICES_END 0x3FFFFFFF
#define HEAP_START 0x20000000
#define HEAP_END 0x24000000

// We do conversions between bitfields and full types all the time around here,
// not worth the extra reinterpret_cast<>(*) everywhere...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

namespace mmu {

struct VirtualAddress {
    VirtualAddress(PtrData ptr)
        : m_ptr(ptr) {};
    VirtualAddress(void* ptr)
        : m_ptr(reinterpret_cast<PtrData>(ptr)) {};

    /* Note: Table entries are 4 bytes */
    int l1_index() const { return m_as.l1_table_index; }
    int l2_index() const { return m_as.l2_table_index; }

    void* ptr() const { return reinterpret_cast<void*>(m_ptr); }

    operator void*() const { return ptr(); }

private:
    struct As {
        u32 _ : 10;
        u32 l2_table_index : 10;
        u32 l1_table_index : 12;
    };
    union {
        As m_as;
        PtrData m_ptr;
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

enum class L1Type : u32 {
    Fault = 0,
    L2Ptr = 1,
    Section = 2,
    SuperSection = 3,
};

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
};

union L1Entry {
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

private:
    Fault _;
    L2Ptr as_ptr;
    Section as_section;
    SuperSection as_super_section;
};

struct L1Table {
    static constexpr auto num_entries = 4096;

    void reserve_section(VirtualAddress, Section);

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

#pragma GCC diagnostic pop

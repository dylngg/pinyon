#include "mmu.hpp"
#include "panic.hpp"

#include <pine/twomath.hpp>
#include <pine/barrier.hpp>
#include <pine/page.hpp>

static_assert(PageSize == mmu::L2Entry::vm_size);
static_assert(HugePageSize == mmu::L1Entry::vm_size);

namespace mmu {

static PhysicalPageAllocator g_physical_page_allocator;  // dummy; ctor not called
static VirtualPageAllocator g_virtual_page_allocator;  // dummy; ctor not called
static PageAllocator g_page_allocator;  // dummy; ctor not called

extern "C" {

void init_page_tables(PtrData code_end)
{
    auto code_huge_page_end = pine::align_up_two(code_end, HugePageSize);

    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    auto code_region = HugePageRegion::from_range(0, code_huge_page_end);

    PageRegion vm_region            { 0,                        MEMORY_END_PAGES };
    HugePageRegion l1_region        { code_region.end_offset(), 1 };
    HugePageRegion scratch_region   { l1_region.end_offset(), 1 };  // 2 each; *PageAllocator requires 2
    auto [pv_scratch_region, page_scratch_region] = as_page_region(scratch_region).halve();
    auto [phys_scratch_region, virt_scratch_region] = pv_scratch_region.halve();
    auto device_region = HugePageRegion::from_range(DEVICES_START, DEVICES_END);
    auto heap_region = HugePageRegion ::from_range(HEAP_START, HEAP_END);

    g_physical_page_allocator.init(vm_region, phys_scratch_region);
    g_virtual_page_allocator.init(vm_region, virt_scratch_region);

    // Map into L1 table
    auto& l1 = *new(static_cast<L1Table*>(l1_region.ptr())) L1Table();
    g_page_allocator.init(l1, g_physical_page_allocator, g_virtual_page_allocator, page_scratch_region);

    g_page_allocator.reserve_huge_page_region(code_region, PageAllocator::Backing::Identity);
    g_page_allocator.reserve_huge_page_region(l1_region, PageAllocator::Backing::Identity);

    g_page_allocator.reserve_huge_page_region(scratch_region);

    // Map devices in upper address space to themselves
    g_page_allocator.reserve_huge_page_region(device_region, PageAllocator::Backing::Identity);

    // FIXME: This is a waste... we should manage this memory somehow...
    g_page_allocator.reserve_huge_page_region(heap_region, PageAllocator::Backing::Identity);

    set_l1_table(l1);
}
}

void print_with(pine::Printer& printer, const L2Ptr& l2_ptr)
{
    constexpr auto bufsize = 64;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, "L2(sbz:%d%d%d, domain:%d%d%d%d, p:%d, addr:%x)",
                     (l2_ptr.sbz & 1), (l2_ptr.sbz & (1 << 2)), (l2_ptr.sbz & (1 << 3)),
                     (l2_ptr.domain & 1), (l2_ptr.domain & (1 << 2)), (l2_ptr.domain & (1 << 3)), (l2_ptr.domain & (1 << 4)),
                     l2_ptr.p,
                     l2_ptr.physical_address().ptr_data());
    print_with(printer, buf);
}

void print_with(pine::Printer& printer, const HugePage& s)
{
    constexpr auto bufsize = 128;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, "HugePage(bc:%d%d, xn:%d, domain:%d%d%d%d, pap:%d%d%d, tex:%d%d%d, apx:%d, s:%d, nG:%d, sbz:%d, addr:%x)",
                     s.b, s.c,
                     s.xn,
                     (s.domain & 1), (s.domain & (1 << 2)), (s.domain & (1 << 3)), (s.domain & (1 << 4)),
                     s.p, (s.ap & 1), (s.ap & (1 << 2)),
                     (s.tex & 1), (s.tex & (1 << 2)), (s.tex & (1 << 3)),
                     s.apx,
                     s.s,
                     s.nG,
                     s.sbz,
                     s.physical_address().ptr_data());
    print_with(printer, buf);
}

void print_with(pine::Printer& printer, const SuperSection& ss)
{
    constexpr auto bufsize = 128;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, "SuperSection(bc:%d%d, xn:%d, domain:%d%d%d%d, pap:%d%d%d, tex:%d%d%d, apx:%d, s:%d, nG:%d, sbz:%d, addr:%x)",
                     ss.b, ss.c,
                     ss.xn,
                     (ss.domain & 1), (ss.domain & (1 << 2)), (ss.domain & (1 << 3)), (ss.domain & (1 << 4)),
                     ss.p, (ss.ap & 1), (ss.ap & (1 << 2)),
                     (ss.tex & 1), (ss.tex & (1 << 2)), (ss.tex & (1 << 3)),
                     ss.apx,
                     ss.s,
                     ss.nG,
                     ss.sbz,
                     ss.physical_address().ptr_data());

    print_with(printer, buf);
}

void print_with(pine::Printer& printer, const L1Table& table)
{
    print_with(printer, "Physical|Entry Addr|Entry\n");
    for (unsigned i = 0; i < mmu::L1Table::num_entries; i++) {
        print_with(printer, VirtualAddress::from_l1_index(i).ptr());
        print_with(printer, "|");

        auto& entry = table.m_entries[i];
        print_with(printer, &entry);
        print_with(printer, "|");

        switch(entry.type()) {
        case L1Type::Fault:
            print_with(printer, "fault");
            break;
        case L1Type::L2Ptr:
            print_with(printer, "\n");
            print_with(printer, *entry.as_ptr.l2_table());
            break;
        case L1Type::HugePage:
            print_with(printer, entry.as_huge_page);
            break;
        case L1Type::SuperSection:
            print_with(printer, entry.as_super_huge_page);
            break;
        }

        print_with(printer, "\n");
    }
}

void print_with(pine::Printer& printer, const L2Table& table)
{
    print_with(printer, "\tPhysical|Entry Addr|Entry\n");
    for (unsigned i = 0; i < mmu::L2Table::num_entries; i++) {
        print_with(printer, "\t");
        print_with(printer, VirtualAddress::from_l2_index(i).ptr());
        print_with(printer, "|");

        auto& entry = table.m_entries[i];
        print_with(printer, &entry);
        print_with(printer, "|");

        switch(entry.type()) {
        case L2Type::Fault:
            print_with(printer, "fault");
            break;
        case L2Type::Page:
            print_with(printer, "page");
            break;
        case L2Type::LargePage:
            print_with(printer, "large_page");
            break;
        }

        print_with(printer, "\n");
    }
}

void set_l1_table(L1Table& l1_addr)
{
    // See B4-1721 in ARMv7 reference manual
    // FIXME: See l1_table() for 14 constant problem; should we issue a dsb here?
    u32 ttbr0 = PhysicalAddress(&l1_addr).ptr_data() & ~(1u << 14u);
    asm volatile("MCR p15, 0, %0, c2, c0, 0" ::"r"(ttbr0));
    pine::DataBarrier::sync();
}

L1Table& l1_table()
{
    // See B4-1721 in ARMv7 reference manual
    PtrData ttbr0;
    asm volatile("MRC p15, 0, %0, c2, c0, 0"
                 : "=r"(ttbr0));
    // FIXME: This should technically be 14 - TTBCR.N, where 2^N controls the
    //        alignment requirements for the L1 translation table. See B4-1722
    //        in the ARMv7 reference manual for technical details, 9.7.2 in the
    //        Cortex-A programmer's guide for background
    return *reinterpret_cast<L1Table*>( (ttbr0 & ~(1u << 14u)) );
}

void PageAllocator::init(L1Table& l1_table, PhysicalPageAllocator& physical_page_allocator, VirtualPageAllocator& virtual_page_allocator, PageRegion scratch_pages)
{
    m_l1_table = &l1_table;
    m_physical_page_allocator = &physical_page_allocator;
    m_virtual_page_allocator = &virtual_page_allocator;
    m_l2_table_allocator.add(scratch_pages.ptr(), scratch_pages.size());
}

Pair<PageRegion, PageRegion> PageAllocator::reserve_region(PageRegion region, PageAllocator::Backing backing)
{
    auto [phys_alloc, virt_alloc] = try_reserve_region_unrecorded(region, backing);
    if (!phys_alloc)
        return {};

    auto phys_region = PageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);
    auto virt_region = PageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);

    if (!try_record_page_in_l1(phys_region, virt_region)) {
        m_physical_page_allocator->free(phys_alloc);
        m_virtual_page_allocator->free(phys_alloc);
    }

    return { phys_region, virt_region };
}

Pair<HugePageRegion, HugePageRegion> PageAllocator::reserve_huge_page_region(HugePageRegion region, PageAllocator::Backing backing)
{
    auto [phys_alloc, virt_alloc] = try_reserve_region_unrecorded(as_page_region(region), backing);
    if (!phys_alloc)
        return {};

    auto phys_region = HugePageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);
    auto virt_region = HugePageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);

    if (!try_record_huge_page_in_l1(phys_region, virt_region)) {
        m_physical_page_allocator->free(phys_alloc);
        m_virtual_page_allocator->free(phys_alloc);
    }

    return { phys_region, virt_region };
}

Pair<PageRegion, PageRegion> PageAllocator::allocate_pages(unsigned int num_pages, pine::PageAlignmentLevel alignment, Backing backing)
{
    auto [phys_alloc, virt_alloc] = try_reserve_page_unrecorded(num_pages, alignment, backing);
    if (!phys_alloc)
        return {};

    auto phys_region = PageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);
    auto virt_region = PageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);

    if (!try_record_page_in_l1(phys_region, virt_region)) {
        m_physical_page_allocator->free(phys_alloc);
        m_virtual_page_allocator->free(phys_alloc);
    }

    return { phys_region, virt_region };
}

Pair<pine::Allocation, pine::Allocation> PageAllocator::try_reserve_page_unrecorded(unsigned num_pages, pine::PageAlignmentLevel alignment, Backing backing)
{
    auto phys_alloc = m_physical_page_allocator->allocate(num_pages, alignment);
    if (!phys_alloc)
        return {};

    auto phys_region = PageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);

    pine::Allocation virt_alloc;
    switch (backing) {
    case Backing::Mixed:
        virt_alloc = m_virtual_page_allocator->allocate(num_pages);
        break;
    case Backing::Identity:
        virt_alloc = m_virtual_page_allocator->reserve_region(phys_region);
        if (!virt_alloc)
            panic(m_virtual_page_allocator);
        break;
    }

    if (!virt_alloc) {
        m_physical_page_allocator->free(phys_alloc);
        return {};
    }

    return { phys_alloc, virt_alloc };
}

Pair<pine::Allocation, pine::Allocation> PageAllocator::try_reserve_region_unrecorded(PageRegion region, Backing backing)
{
    auto phys_alloc = m_physical_page_allocator->reserve_region(region);
    if (!phys_alloc)
        return {};

    auto phys_region = PageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);

    pine::Allocation virt_alloc;
    switch (backing) {
    case Backing::Mixed:
        virt_alloc = m_virtual_page_allocator->allocate(region.length);
        break;
    case Backing::Identity:
        virt_alloc = m_virtual_page_allocator->reserve_region(phys_region);
        break;
    }

    if (!virt_alloc) {
        m_physical_page_allocator->free(phys_alloc);
        return {};
    }

    return { phys_alloc, virt_alloc };
}

bool PageAllocator::try_record_huge_page_in_l1(HugePageRegion phys_region, HugePageRegion virt_region)
{
    if (phys_region.length != virt_region.length)
        return false;  // FIXME: Assert?

    for (unsigned l1_offset = 0; l1_offset < virt_region.length; l1_offset++) {
        auto virt_ptr = virt_region.ptr(l1_offset);
        auto phys_ptr = phys_region.ptr(l1_offset);

        auto &l1_entry = m_l1_table->retrieve_entry(virt_ptr);
        switch (l1_entry.type()) {
        case L1Type::HugePage:
        case L1Type::SuperSection:
        case L1Type::L2Ptr:
            panic("Tried to record huge page that was already recorded:", virt_region.ptr(), *m_physical_page_allocator,
                  "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
            return false;
        case L1Type::Fault:
            l1_entry = HugePage(PhysicalAddress(phys_ptr));
            break;
        }
    }
    return true;
}

bool PageAllocator::try_record_page_in_l1(PageRegion phys_region, PageRegion virt_region, void* l2_backing)
{
    unsigned num_l2_tables_to_walk = pine::divide_up(virt_region.length, static_cast<unsigned>(L2Table::num_entries));
    unsigned num_l2_tables_per_l1 = L2Table::num_entries;

    for (unsigned l1_offset = 0; l1_offset < num_l2_tables_to_walk; l1_offset++) {
        auto& l1_entry = m_l1_table->retrieve_entry(virt_region.ptr(l1_offset*num_l2_tables_per_l1));

        auto* maybe_l2_table = static_cast<L2Table*>(l2_backing);  // could be nullptr
        switch (l1_entry.type()) {
        case L1Type::HugePage:
        case L1Type::SuperSection:
            panic("Tried to record huge page that was already recorded:", virt_region.ptr(l1_offset), *m_physical_page_allocator, "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
            return false;

        case L1Type::L2Ptr:
            maybe_l2_table = l1_entry.as_ptr.l2_table();
            break;

        case L1Type::Fault: {
            if (!l2_backing) {
                auto l2_table_alloc = try_reserve_l2_table_entry();
                if (!l2_table_alloc)
                    return false;

                maybe_l2_table = reinterpret_cast<L2Table*>(l2_table_alloc.ptr);
            }

            new (maybe_l2_table) L2Table();
            l1_entry = L2Ptr(PhysicalAddress(maybe_l2_table));
            break;
        }
        }

        for (unsigned l2_offset = 0; l2_offset < num_l2_tables_per_l1 && l1_offset*num_l2_tables_to_walk + l2_offset < virt_region.length; l2_offset++) {
            auto virt_ptr = virt_region.ptr(l1_offset * num_l2_tables_to_walk + l2_offset);
            auto phys_ptr = phys_region.ptr(l1_offset * num_l2_tables_to_walk + l2_offset);

            auto& l2_entry = maybe_l2_table->retrieve_entry(virt_ptr);
            switch (l2_entry.type()) {
            case L2Type::Fault:
                l2_entry = Page(PhysicalAddress(phys_ptr));
                break;
            default:
                panic("Tried to record page that was already recorded:", virt_region.ptr(), *m_physical_page_allocator, "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
                return false;
            }
        }
    }
    return true;
}

pine::Allocation PageAllocator::try_reserve_l2_table_entry()
{
    auto l2_alloc = m_l2_table_allocator.allocate(sizeof(L2Table));
    if (l2_alloc)
        return l2_alloc;

    // Use spare as backing for new allocation
    auto [alloc, _] = try_reserve_page_unrecorded(1, pine::PageAlignmentLevel::Page, Backing::Identity);
    if (!alloc)
        return {};  // Completely out of memory!

    auto region = PageRegion::from_ptr(alloc.ptr, alloc.size);
    // Should always succeed since we have a backing
    PANIC_IF(!try_record_page_in_l1(region, region, m_spare_free_l2_page));

    m_l2_table_allocator.add(alloc.ptr, alloc.size);

    // Priority is allocation not another space page, so alloc that first.
    // That being said, it is unlikely both will fail
    alloc = m_l2_table_allocator.allocate(sizeof(L2Table));
    m_spare_free_l2_page = m_l2_table_allocator.allocate(sizeof(L2Table)).ptr;
    return alloc;
}

}


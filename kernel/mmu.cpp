#include "mmu.hpp"
#include "panic.hpp"

#include <pine/twomath.hpp>
#include <pine/barrier.hpp>
#include <pine/page.hpp>

static_assert(PageSize == mmu::L2Entry::vm_size);
static_assert(SectionSize == mmu::L1Entry::vm_size);

namespace mmu {

static PhysicalPageAllocator g_physical_page_allocator;  // dummy; ctor not called
static VirtualPageAllocator g_virtual_page_allocator;  // dummy; ctor not called
static PageAllocator g_page_allocator;  // dummy; ctor not called

PageAllocator& page_allocator()
{
    return g_page_allocator;
}
extern "C" {

void init_page_tables(PtrData code_end)
{
    auto code_section_end = pine::align_up_two(code_end, SectionSize);

    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    auto code_region = SectionRegion::from_range(0, code_section_end);

    PageRegion vm_region         { 0,                        MEMORY_END_PAGES };
    SectionRegion l1_region      { code_region.end_offset(), 1 };
    // Experimental trials suggests 16 is the min here; at least 4 pages are required for the
    // physical and virtual page allocators
    SectionRegion scratch_region { l1_region.end_offset(), 1 };
    auto [pv_scratch_region, page_scratch_region] = as_page_region(scratch_region).halve();
    auto [phys_scratch_region, virt_scratch_region] = pv_scratch_region.halve();
    auto device_region = SectionRegion::from_range(DEVICES_START, DEVICES_END);

    g_physical_page_allocator.init(vm_region, phys_scratch_region);
    g_virtual_page_allocator.init(vm_region, virt_scratch_region);

    // Map into L1 table
    auto& l1 = *new(static_cast<L1Table*>(l1_region.ptr())) L1Table();
    g_page_allocator.init(l1, g_physical_page_allocator, g_virtual_page_allocator, page_scratch_region);

    g_page_allocator.reserve_section_region(code_region, PageAllocator::Backing::Identity);
    g_page_allocator.reserve_section_region(l1_region, PageAllocator::Backing::Identity);

    g_page_allocator.reserve_section_region(scratch_region);

    // Map devices in upper address space to themselves
    g_page_allocator.reserve_section_region(device_region, PageAllocator::Backing::Identity);

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

void print_with(pine::Printer& printer, const Section& s)
{
    constexpr auto bufsize = 128;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, "Section(bc:%d%d, xn:%d, domain:%d%d%d%d, pap:%d%d%d, tex:%d%d%d, apx:%d, s:%d, nG:%d, sbz:%d, addr:%x)",
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
    print_with(printer, "Virtual|Physical|Entry Addr|Entry\n");
    for (unsigned i = 0; i < mmu::L1Table::num_entries; i++) {
        print_with(printer, VirtualAddress::from_l1_index(i).ptr());
        print_with(printer, "|");

        auto& entry = table.m_entries[i];
        switch(entry.type()) {
        case L1Type::Fault:
            print_with(printer, "fault");
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, "fault");
            break;

        case L1Type::L2Ptr:
            print_with(printer, entry.as_ptr.physical_address().ptr());
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "\n");
            print_with(printer, *entry.as_ptr.l2_table());
            break;

        case L1Type::Section:
            print_with(printer, entry.as_section.physical_address().ptr());
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, entry.as_section);
            break;

        case L1Type::SuperSection:
            print_with(printer, entry.as_super_section.physical_address().ptr());
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, entry.as_super_section);
            break;
        }

        print_with(printer, "\n");
    }
}

void print_with(pine::Printer& printer, const L2Table& table)
{
    print_with(printer, "\tVirtual Offset|Physical|Entry Addr|Entry\n");
    for (unsigned i = 0; i < mmu::L2Table::num_entries; i++) {
        print_with(printer, "\t");
        print_with(printer, VirtualAddress::from_l2_index(i).ptr());
        print_with(printer, "|");

        auto& entry = table.m_entries[i];

        switch(entry.type()) {
        case L2Type::Fault:
            print_with(printer, "fault");
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, "fault");
            break;
        case L2Type::Page:
            print_with(printer, entry.as_page.physical_address().ptr());
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, "page");
            break;
        case L2Type::HugePage:
            print_with(printer, entry.as_huge_page.physical_address().ptr());
            print_with(printer, "|");
            print_with(printer, &entry);
            print_with(printer, "|");
            print_with(printer, "huge_page");
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

pine::Allocation PageAllocator::allocate(size_t size)
{
    auto num_pages = pine::align_up_two(size, PageSize) / PageSize;
    auto [_, virt_region] = allocate_pages(num_pages);
    return { virt_region.ptr(), virt_region.size() };
}

void PageAllocator::free(pine::Allocation alloc)
{
    // FIXME: Todo!
    panic("Tried to free memory given by the PageAllocator!");
}

Pair<PageRegion, PageRegion> PageAllocator::reserve_region(PageRegion region, PageAllocator::Backing backing)
{
    auto [phys_alloc, virt_alloc] = try_reserve_region_unrecorded(region, backing);
    if (!phys_alloc)
        return {};

    auto phys_region = PageRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);
    auto virt_region = PageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);
    PANIC_IF(virt_region.length != phys_region.length);

    if (!try_record_page_in_l1(phys_region, virt_region)) {
        m_physical_page_allocator->free(phys_alloc);
        m_virtual_page_allocator->free(phys_alloc);
    }

    return { phys_region, virt_region };
}

Pair<SectionRegion, SectionRegion> PageAllocator::reserve_section_region(SectionRegion region, PageAllocator::Backing backing)
{
    auto [phys_alloc, virt_alloc] = try_reserve_region_unrecorded(as_page_region(region), backing);
    if (!phys_alloc)
        return {};

    auto phys_region = SectionRegion::from_ptr(phys_alloc.ptr, phys_alloc.size);
    auto virt_region = SectionRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);

    if (!try_record_section_in_l1(phys_region, virt_region)) {
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
    auto virt_alloc = m_virtual_page_allocator->reserve_region(region);
    if (!virt_alloc)
        return {};

    auto virt_region = PageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);

    pine::Allocation phys_alloc;
    switch (backing) {
    case Backing::Mixed:
        phys_alloc = m_physical_page_allocator->allocate(region.length);
        break;
    case Backing::Identity:
        phys_alloc = m_physical_page_allocator->reserve_region(virt_region);
        break;
    }

    if (!phys_alloc) {
        m_virtual_page_allocator->free(virt_alloc);
        return {};
    }

    return { phys_alloc, virt_alloc };
}

bool PageAllocator::try_record_section_in_l1(SectionRegion phys_region, SectionRegion virt_region)
{
    if (phys_region.length != virt_region.length)
        return false;  // FIXME: Assert?

    for (unsigned l1_offset = 0; l1_offset < virt_region.length; l1_offset++) {
        auto virt_ptr = virt_region.ptr(l1_offset);
        auto phys_ptr = phys_region.ptr(l1_offset);

        auto &l1_entry = m_l1_table->retrieve_entry(virt_ptr);
        switch (l1_entry.type()) {
        case L1Type::Section:
        case L1Type::SuperSection:
        case L1Type::L2Ptr:
            panic("Tried to record huge page that was already recorded:", virt_region.ptr(), *m_physical_page_allocator,
                  "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
            return false;
        case L1Type::Fault:
            l1_entry = Section(PhysicalAddress(phys_ptr));
            break;
        }
    }
    return true;
}

bool PageAllocator::try_record_page_in_l1(PageRegion phys_region, PageRegion virt_region, void* l2_backing)
{
    unsigned num_l1_entries_to_walk = pine::divide_up(virt_region.size(), L1Entry::vm_size);
    constexpr unsigned num_l2_tables = L2Table::num_entries;

    for (unsigned l1_offset = 0; l1_offset < num_l1_entries_to_walk; l1_offset++) {
        auto* virt_ptr = virt_region.ptr(l1_offset * num_l2_tables);
        auto& l1_entry = m_l1_table->retrieve_entry(virt_ptr);

        auto* maybe_l2_table = static_cast<L2Table*>(l2_backing);  // could be nullptr
        switch (l1_entry.type()) {
        case L1Type::Section:
        case L1Type::SuperSection:
            panic("Tried to record huge page that was already recorded:", virt_ptr, *m_physical_page_allocator, "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
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

        for (unsigned l2_offset = 0; l2_offset < num_l2_tables && l1_offset * num_l2_tables + l2_offset < virt_region.length; l2_offset++) {
            auto* virt_ptr = virt_region.ptr(l1_offset * num_l2_tables + l2_offset);
            auto* phys_ptr = phys_region.ptr(l1_offset * num_l2_tables + l2_offset);

            auto& l2_entry = maybe_l2_table->retrieve_entry(virt_ptr);
            switch (l2_entry.type()) {
            case L2Type::Fault:
                l2_entry = Page(PhysicalAddress(phys_ptr));
                break;
            default:
                panic("Tried to record page that was already recorded:", virt_ptr, *m_physical_page_allocator, "\n", *m_virtual_page_allocator, "\n", *m_l1_table);
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


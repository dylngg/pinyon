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

PhysicalPageAllocator& physical_page_allocator()
{
    return g_physical_page_allocator;
}

static bool reserve_identity_section(L1Table* l1_table, SectionRegion region)
{
    auto phys_alloc = g_physical_page_allocator.reserve_region(as_page_region(region));
    if (!phys_alloc)
        return false;

    auto virt_alloc = g_virtual_page_allocator.reserve_region(as_page_region(region));
    if (!virt_alloc)
        return false;

    l1_table->reserve_identity_section_region(region);

    return true;
}


static bool reserve_backed_section(L1Table* l1_table, SectionRegion region)
{
    auto phys_alloc = g_physical_page_allocator.allocate_section(region.length);
    if (!phys_alloc)
        return false;

    auto virt_alloc = g_virtual_page_allocator.reserve_region(as_page_region(region));
    if (!virt_alloc)
        return false;

    auto virt_region = SectionRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);
    l1_table->reserve_section_region(region, virt_region);

    return true;
}

extern "C" {

void init_page_tables(PtrData code_end)
{
    auto code_section_end = pine::align_up_two(code_end, HugePageSize);

    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    auto code_region = SectionRegion::from_range(0, code_section_end);
    SectionRegion l1_region { code_region.end_offset(), 1 };
    SectionRegion phys_scratch_region {l1_region.end_offset(), 1 };
    SectionRegion virt_scratch_region {phys_scratch_region.end_offset(), 1 };
    PageRegion vm_region { as_page_region(virt_scratch_region).end_offset(), MEMORY_END_PAGES-as_page_region(virt_scratch_region).offset };
    g_physical_page_allocator.init(vm_region, as_page_region(phys_scratch_region));
    g_virtual_page_allocator.init(vm_region, as_page_region(virt_scratch_region));

    // Map into L1 table
    auto* l1 = new(static_cast<L1Table*>(l1_region.ptr())) L1Table();
    l1->reserve_identity_section_region(code_region);
    l1->reserve_identity_section_region(l1_region);
    l1->reserve_identity_section_region(phys_scratch_region);
    l1->reserve_identity_section_region(virt_scratch_region);

    // Map devices in upper address space to themselves
    auto device_region = SectionRegion::from_range(DEVICES_START, DEVICES_END);
    PANIC_IF(!reserve_identity_section(l1, device_region));

    // FIXME: This is a waste... we should manage this memory somehow...
    auto heap_region = SectionRegion::from_range(HEAP_START, HEAP_END);
    PANIC_IF(!reserve_backed_section(l1, heap_region));

    set_l1_table(PhysicalAddress(l1));
}
}

void L1Table::reserve_section(VirtualAddress virt_addr, Section section)
{
    auto ptr_data = section.physical_address().ptr_data();
    PANIC_IF(!pine::is_aligned_two(ptr_data, L1Entry::vm_size));
    m_entries[virt_addr.l1_index()] = section;
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
    print_with(printer, "Physical|Entry\n");
    for (int i = 0; i < table.num_entries; i++) {
        print_with(printer, VirtualAddress::from_l1_index(i).ptr());
        print_with(printer, "|");

        auto& entry = table.m_entries[i];
        switch(entry.type()) {
        case L1Type::Fault:
            print_with(printer, "fault");
            break;
        case L1Type::L2Ptr:
            print_with(printer, entry.as_ptr);
            break;
        case L1Type::Section:
            print_with(printer, entry.as_section);
            break;
        case L1Type::SuperSection:
            print_with(printer, entry.as_super_section);
            break;
        }
        print_with(printer, "\n");
    }
}

void set_l1_table(PhysicalAddress l1_addr)
{
    // See B4-1721 in ARMv7 reference manual
    // FIXME: See l1_table() for 14 constant problem; should we issue a dsb here?
    u32 ttbr0 = l1_addr.ptr_data() & ~(1 << 14);
    asm volatile("MCR p15, 0, %0, c2, c0, 0" ::"r"(ttbr0));
    pine::DataBarrier::sync();
}

PhysicalAddress l1_table()
{
    // See B4-1721 in ARMv7 reference manual
    u32 ttbr0;
    asm volatile("MRC p15, 0, %0, c2, c0, 0"
                 : "=r"(ttbr0));
    // FIXME: This should technically be 14 - TTBCR.N, where 2^N controls the
    //        alignment requirements for the L1 translation table. See B4-1722
    //        in the ARMv7 reference manual for technical details, 9.7.2 in the
    //        Cortex-A programmer's guide for background
    return PhysicalAddress { (ttbr0 & ~(1 << 14u)) };
}

}

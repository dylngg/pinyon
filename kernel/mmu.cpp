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
VirtualPageAllocator & virtual_page_allocator()
{
    return g_virtual_page_allocator;
}

extern "C" {

void init_page_tables(PtrData code_end)
{
    auto code_huge_page_end = pine::align_up_two(code_end, HugePageSize);

    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    auto code_region = HugePageRegion::from_range(0, code_huge_page_end);
    HugePageRegion l1_region {code_region.end_offset(), 1 };
    HugePageRegion phys_scratch_region {l1_region.end_offset(), 1 };
    HugePageRegion virt_scratch_region {phys_scratch_region.end_offset(), 1 };
    PageRegion vm_region { as_page_region(virt_scratch_region).end_offset(), MEMORY_END_PAGES-as_page_region(virt_scratch_region).offset };
    g_physical_page_allocator.init(vm_region, as_page_region(phys_scratch_region));
    g_virtual_page_allocator.init(vm_region, as_page_region(virt_scratch_region));

    // Map into L1 table
    auto& l1 = *new(static_cast<L1Table*>(l1_region.ptr())) L1Table();
    l1.reserve_identity_huge_page_region(code_region);
    l1.reserve_identity_huge_page_region(l1_region);
    l1.reserve_identity_huge_page_region(phys_scratch_region);
    l1.reserve_identity_huge_page_region(virt_scratch_region);

    // Map devices in upper address space to themselves
    auto device_region = HugePageRegion::from_range(DEVICES_START, DEVICES_END);
    PANIC_IF(!reserve_identity_huge_page(l1, device_region));

    // FIXME: This is a waste... we should manage this memory somehow...
    auto heap_region = HugePageRegion::from_range(HEAP_START, HEAP_END);
    PANIC_IF(!reserve_backed_huge_page(l1, heap_region));

    set_l1_table(l1);
}
}

void L1Table::reserve_huge_page(VirtualAddress virt_addr, HugePage huge_page)
{
    auto ptr_data = huge_page.physical_address().ptr_data();
    PANIC_IF(!pine::is_aligned_two(ptr_data, L1Entry::vm_size));
    m_entries[virt_addr.l1_index()] = huge_page;
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

bool reserve_identity_huge_page(L1Table& l1_table, HugePageRegion region)
{
    auto phys_alloc = physical_page_allocator().reserve_region(as_page_region(region));
    if (!phys_alloc)
        return false;

    auto virt_alloc = physical_page_allocator().reserve_region(as_page_region(region));
    if (!virt_alloc)
        return false;

    l1_table.reserve_identity_huge_page_region(region);
    return true;
}

bool reserve_backed_huge_page(L1Table& l1_table, HugePageRegion region)
{
    auto phys_alloc = physical_page_allocator().allocate(region.length, pine::PageAlignmentLevel::HugePage);
    if (!phys_alloc)
        return false;

    auto virt_alloc = virtual_page_allocator().reserve_region(as_page_region(region));
    if (!virt_alloc)
        return false;

    auto virt_region = HugePageRegion::from_ptr(virt_alloc.ptr, virt_alloc.size);
    l1_table.reserve_huge_page_region(region, virt_region);
    return true;
}

}



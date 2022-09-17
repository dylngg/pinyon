#include "mmu.hpp"
#include "panic.hpp"

#include <pine/twomath.hpp>
#include <pine/barrier.hpp>
#include <pine/page.hpp>

static_assert(PageSize == mmu::L2Entry::vm_size);
static_assert(HugePageSize == mmu::L1Entry::vm_size);

namespace mmu {

static PhysicalPageAllocator g_physical_page_allocator;  // dummy; ctor not called

PhysicalPageAllocator& physical_page_allocator()
{
    return g_physical_page_allocator;
}

extern "C" {

void init_page_tables(PtrData code_end)
{
    auto code_section_end = pine::align_up_two(code_end, HugePageSize);

    Region vm_region { 0, MEMORY_END_PAGES };
    Region scratch { code_section_end / PageSize, HugePageSize };
    g_physical_page_allocator.init(vm_region, scratch);

    // Reserve from 0 to code end first, so not used for L1 table
    auto code_alloc = g_physical_page_allocator.reserve_region(Region {0, code_section_end / PageSize });
    PANIC_MESSAGE_IF(!code_alloc, "Failed to reserve code region in MMU!");

    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    // Reserve L1 table itself
    auto l1_alloc = g_physical_page_allocator.allocate_section(1);
    auto* l1 = new(static_cast<L1Table*>(l1_alloc.ptr)) L1Table();
    PANIC_MESSAGE_IF(!l1_alloc, "Failed to reserve L1 code region in MMU!");
    l1->reserve_section(l1, PhysicalAddress(l1));

    // Reserve code as L1 blocks; start at 0x0 because that is where the vector
    // tables, then mode stacks live.
    for (PtrData addr = 0; addr < code_section_end / HugePageSize; addr += HugePageSize)
        l1->reserve_section(addr, PhysicalAddress(addr));

    // Map devices in upper address space to themselves
    for (PtrData addr = DEVICES_START; addr <= DEVICES_END; addr += HugePageSize)
        l1->reserve_section(addr, PhysicalAddress(addr));

    // FIXME: This is a waste... we should manage this memory somehow...
    auto l1_region = Region::from_ptr(l1_alloc.ptr, l1_alloc.size);

    auto heap_phys_addr = PhysicalAddress(l1_region.end_ptr());
    for (PtrData region = HEAP_START; region <= HEAP_END; region += HugePageSize) {
        l1->reserve_section(region, heap_phys_addr);
        heap_phys_addr += HugePageSize;
    }

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

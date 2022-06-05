#include "mmu.hpp"
#include "panic.hpp"

#include <pine/twomath.hpp>
#include <pine/barrier.hpp>

namespace mmu {

extern "C" {

void init_page_tables(PtrData code_end)
{
    // FIXME: We are wasting a lot of space with 1MiB alignment for code and
    //        especially for 16KiB L1Table
    auto code_end_l1_aligned = pine::align_up_two(code_end, L1Entry::vm_size);
    auto* l1 = reinterpret_cast<L1Table*>(code_end_l1_aligned);

    // Reserve code as L1 blocks; start at 0x0 because that is where the vector
    // tables, then mode stacks live.
    for (PtrData region = 0x0; region < code_end_l1_aligned; region += L1Entry::vm_size)
        l1->reserve_section(region, Section { PhysicalAddress { region } });

    // Reserve L1 table itself
    l1->reserve_section(l1, Section { PhysicalAddress { l1 } });

    // Map devices in upper address space to themselves
    for (PtrData region = DEVICES_START; region <= DEVICES_END; region += L1Entry::vm_size)
        l1->reserve_section(region, Section { PhysicalAddress { region } });

    // FIXME: This is a waste... we should manage this memory somehow...
    auto heap_phys_addr = PhysicalAddress { pine::align_up_two((PtrData)l1 + 1, L1Entry::vm_size) };
    for (PtrData region = HEAP_START; region <= HEAP_END; region += L1Entry::vm_size) {
        l1->reserve_section(region, Section { heap_phys_addr });
        heap_phys_addr += L1Entry::vm_size;
    }

    set_l1_table(PhysicalAddress { l1 });
}
}

void L1Table::reserve_section(VirtualAddress virt_addr, Section section)
{
    auto ptr_data = section.physical_address().ptr_data();
    PANIC_IF(!pine::is_aligned_two(ptr_data, L1Entry::vm_size));
    m_entries[virt_addr.l1_index()] = section;
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

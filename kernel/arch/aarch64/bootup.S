.section .boot

.global start
.global halt

scr_el3_value = 0x30d00002

start:
    /* -- Disable all but one core --*/
    /*
     * Read our CPU ID from MPIDR (multiprocessor affinity register) into x0;
     * e.g. for 4 processors we'll have CPU ID: 0x0, 0x1, 0x2, 0x3
     */
    mrs x0, mpidr_el1
    and x0, x0, #3      /* r0 &= 0x3 */
    cbz x0, core_zero_start
    ldr x1, =halt
    br x1

core_zero_start:
    /* -- Make sure we are running at EL1 -- */
    mrs x0, currentel
    and x0, x0, #12     /* bits 2:3 */
    lsr x0, x0, #2
    cmp x0, #3          /* EL3? */
    bne check_if_el2
step_down_el3:
    /*
     * There are two ways of changing the exception level: take an exception
     * and return from an exception. The latter is how we step down our level.
     *
     * the ERET instruction, which does this stepping down, returns to the
     * address inside the ELR_ELn register and sets the PSTATE (processor
     * state; see 4.2 in ARMv8 Programmer's Guide) from the SPSR_ELn register.
     */
    mrs x0, scr_el3
    orr x0, x0, #(1 << 10)      /* Set execution state for lower EL to aarch64 (bit 10) */
    msr scr_el3, x0
    adr x1, zero_out_bss        /* Set LR for lower EL */
    msr elr_el3, x1
    mov x2, #0b101      /* bits 0:3; the last bit determines whether to use SP_EL0 or SP_ELn */
    msr spsr_el3, x2    /* Set to EL1; Note that interrupt bits are disabled */
    eret

check_if_el2:
    cmp x0, #2          /* EL2? */
    bne zero_out_bss
step_down_el2:
    adr x1, zero_out_bss
    msr elr_el2, x1
    mov x2, #0b101      /* bits 0:3; the last bit determines whether to use SP_EL0 or SP_ELn */
    msr spsr_el2, x2    /* Set to EL1; Note that interrupt bits are disabled */
    eret

zero_out_bss:
    mrs x0, currentel
    /* -- Zero out BSS (global or static variables) -- */
    ldr x2, =__bss_size
    cbz x2, preinit
zbss:
    str xzr, [x1], #8
    sub x2, x2, #1
    cbnz x2, zbss

preinit:
    ldr x0, =setup_vtable
    blr x0
    ldr x0, =setup_stacks
    blr x0
    ldr x0, =init
    blr x0

halt:
    wfe
    ldr x0, =halt
    br x0

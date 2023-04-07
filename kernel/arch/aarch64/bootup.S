.section .boot

.global start
.global halt
.global halt_addr
.global spin_addr

start:
    /* -- Disable all but one core -- */
    /*
     * Read our CPU ID from MPIDR (multiprocessor affinity register) into x0;
     * e.g. for 4 processors we'll have CPU ID: 0x0, 0x1, 0x2, 0x3
     */
    mrs x0, mpidr_el1
    and x0, x0, #3      /* r0 &= 0x3 */
    cbz x0, check_if_el3
    ldr x1, =halt
    br x1

check_if_el3:
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
    /*
     * bit 0: Exception levels lower then EL3 are in non-secure state
     *   1-3: Disable IRQ, FIQ and external abort and serror for EL3
     *   4-5: Reserved RES1
     *     6: Reserved RES0
     *     7: Disable SMC instructions at EL1 and above
     *     8: Disable HVC instructions at EL1 and above
     *    10: Next lower level (EL2) is aarch64
     */
    mov x0, #0b10110110001
    msr scr_el3, x0
    adr x1, step_down_el2       /* Set LR for lower EL */
    msr elr_el3, x1
    /*
     * bit 0: Whether to switch to ELn H or T
     *   1-3: ELn
     *     4: Execution state 0 for aarch64
     *     5: Reserved, RES0
     *     6: FIQ Interrupt Mask
     *     7: IRQ Interrupt Mask
     *     8: SError Interrupt Mask
     *     9: Debug Interrupt Mask
     */
    mov x2, #0b1111001001
    msr spsr_el3, x2
    eret

check_if_el2:
    cmp x0, #2          /* EL2? */
    bne zero_out_bss
step_down_el2:
    mrs x0, hcr_el2
    orr x0, x0, #(1 << 29)   /* Disable HVC instructions */
    orr x0, x0, #(1 << 31)   /* EL1 is aarch64 */
    msr hcr_el2, x0

    ldr x0, =setup_vtable
    blr x0

    ldr x0, =setup_stacks
    blr x0

    adr x1, zero_out_bss
    msr elr_el2, x1
    /*
     * bit 0: Whether to switch to ELn H or T
     *   1-3: ELn
     *     4: Execution state 0 for aarch64
     *     5: Reserved, RES0
     *     6: FIQ Interrupt Mask
     *     7: IRQ Interrupt Mask
     *     8: SError Interrupt Mask
     *     9: Debug Interrupt Mask
     */
    mov x2, #0b1111000101
    msr spsr_el2, x2    /* Set to EL1; Note that interrupt bits are disabled */
    eret

zero_out_bss:
    /* -- Zero out BSS (global or static variables) -- */
    ldr x1, =__bss_start
    ldr x2, =__bss_end
    b       ckzbssbnds
zbss:
    str xzr, [x1], #8
ckzbssbnds:
    subs x0, x2, x1
    cbnz x0, zbss

preinit:
    ldr x0, =init
    blr x0

spin:
halt:
    wfe
    ldr x0, =halt
    br x0

/*
 * C function that returns the code address of 'halt'.
 *
 * Used in place of 'ldr %0, =halt' elsewhere to get around literal pool
 * locality issues.
 */
halt_addr:
    ldr x0, =halt
    ret

/*
 * C function that returns the code address of 'spin', similar to halt_addr.
 */
spin_addr:
    ldr x0, =spin
    ret

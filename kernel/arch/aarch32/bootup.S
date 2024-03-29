.section .boot

.global start
.global halt
.global halt_addr
.global spin_addr
.global mmu_init

start:
    /* -- Disable all but one core --*/
    /*
     * Read our CPU ID from MPIDR (multiprocessor affinity register) into r3;
     * e.g. for 4 processors we'll have CPU ID: 0x0, 0x1, 0x2, 0x3
     */
    /*
     * Note: p15 is the system control processor,
     *       c0 is coprocessor 0,
     *       #5 indicates MPIDR
     */
    /* See 4.3.5 in ARM Cortex-A7 MPCore Technical Reference */
    mrc p15, #0, r3, c0, c0, #5
    and r3, r3, #3      /* r0 &= 0x3 */
    cmp r3, #0          /* Now "halt" if not core 0 */
    ldr r0, =halt
    bxne r0

    /* -- Zero out BSS (global or static variables) -- */
    ldr r2, =__bss_start
    ldr r7, =__bss_end
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    b       ckzbssbnds
zbss:
    /* Basically zero out 4 words at r2 addr, stm increases r2 to
     * next segment; with !, puts that increase back in r2
     * See https://heyrick.eu/armwiki/STM
     */
    stm r2!, {r3-r6}
ckzbssbnds:
    cmp r2, r7
    blo zbss            /* r2 < r7; r2 is curr addr, r7 is the end of bss */

preinit:
    ldr r0, =setup_vtable
    blx r0
    ldr r0, =init
    blx r0

mmu_init:
    push {r0-r3, r12, lr}

    ldr r0, =__code_end
    ldr r1, =init_page_tables
    blx r1

    mcr p15, 0, r0, c8, c7, 0   /* See B3.18.7; Invalidate entire unified TLB; r0 is dummy */
    dmb
    dsb                         /* Data and instruction barrier */

    mov r2, #3
    mcr p15, 0, r2, c3, c0, 0   /* See B4.1.43; Set manager access (0b11) for domain 0 (we don't use domains currently) */

    mrc p15, 0, r3, c1, c0, 0   /* See B4.1.130; Enable MMU (first bit) in SCTLR */
    dmb
    orr r3, #1
    mcr p15, 0, r3, c1, c0, 0

    pop {r0-r3, r12, pc}

spin:
halt:
    wfi                 /* wait for interrupt; puts processor in low power mode */
    ldr r0, =halt
    bx r0

/*
 * C function that returns the code address of 'halt'.
 *
 * Used in place of 'ldr %0, =halt' elsewhere to get around literal pool
 * locality issues.
 */
halt_addr:
    ldr r0, =halt
    bx lr

/*
 * C function that returns the code address of 'spin', similar to halt_addr.
 */
spin_addr:
    ldr r0, =spin
    bx lr

.section .boot
.global start
.global halt

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
    bne halt

    /* -- Zero out BSS (global or static variables) -- */
    ldr r2, =__bss_start
    ldr r7, =__bss_end
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    b       ckzbssbnds
zbss:
    /* Basically zero out 4 words at r2 addr, ia with stm increases r2 to
     * next segment; with !, puts that increase back in r2
     * See https://heyrick.eu/armwiki/STM
     */
    stmia r2!, {r3-r6}
ckzbssbnds:
    cmp r2, r7
    blo zbss            /* r2 < r7; r2 is curr addr, r7 is the end of bss */

preinit:
    bl setup_vtable     /* Set up vector interrupt table */

    mov sp, #0x8000     /* Set up stack */

    /* -- Load up init in kernel.cpp -- */
    ldr r1, =init
    blx r1

halt:
    wfi                 /* wait for interrupt; puts processor in low power mode */
    b halt

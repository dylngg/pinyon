.section .text
.global setup_vtable
.global setup_stacks

/* FIXME? These should just be macros? We don't change them anywhere. */
kernel_sp = 0xb000

setup_vtable:
    ldr x0, =kernel_vtable
    msr vbar_el1, x0    /* Load vtable address for EL1 */
    ret

setup_stacks:
    /*
     * WARNING! If stacks are changed, you will need to modify bootup.S and linker.ld
     */
    ldr x0, =kernel_sp
    mov sp, x0
    ret

.balign 0x800
kernel_vtable:
    // Synchronous
    .balign 0x80
    ldr x0, =halt
    br x0

    // IRQ
    .balign 0x80
    ldr x0, =halt
    br x0

    // FIQ
    .balign 0x80
    ldr x0, =halt
    br x0

    // SError
    .balign 0x80
    ldr x0, =halt
    br x0

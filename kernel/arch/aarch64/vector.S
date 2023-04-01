.section .text
.global setup_vtable
.global setup_stacks
.global enable_irq

/* FIXME? These should just be macros? We don't change them anywhere. */
/*
 * WARNING! If stacks are changed, you will need to modify bootup.S and linker.ld
 */
kernel_sp = 0xb000

setup_vtable:
    adr x0, vtable
    msr vbar_el1, x0    /* Load vtable address for EL1 */
    ret

setup_stacks:
    ldr x0, =kernel_sp
    msr sp_el1, x0
    ret

enable_irq:
    msr daifclr, #0b0000
    ret

.balign 0x800
vtable:
.balign 0x80
synchronous_kernel_sp0:
    ldr x0, =halt
    br x0

.balign 0x80
irq_kernel_sp0:
    ldr x0, =halt
    br x0

.balign 0x80
fiq_kernel_sp0:
    ldr x0, =halt
    br x0

.balign 0x80
serror_kernel_sp0:
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_kernel:
    sub sp, sp, #(16 * 16)
    stp x0, x1, [sp, #(16 * 0)]
    stp x2, x3, [sp, #(16 * 1)]
    stp x4, x5, [sp, #(16 * 2)]
    stp x6, x7, [sp, #(16 * 3)]
    stp x8, x9, [sp, #(16 * 4)]
    stp x10, x11, [sp, #(16 * 5)]
    stp x12, x13, [sp, #(16 * 6)]
    stp x14, x15, [sp, #(16 * 7)]
    stp x16, x17, [sp, #(16 * 8)]
    stp x18, x19, [sp, #(16 * 9)]
    stp x20, x21, [sp, #(16 * 10)]
    stp x22, x23, [sp, #(16 * 11)]
    stp x24, x25, [sp, #(16 * 12)]
    stp x26, x27, [sp, #(16 * 13)]
    stp x28, x29, [sp, #(16 * 14)]
    stp lr, xzr, [sp, #(16 * 15)]

    mov x0, sp
    bl synchronous_kernel_handler

    ldp x0, x1, [sp, #(16 * 0)]
    ldp x2, x3, [sp, #(16 * 1)]
    ldp x4, x5, [sp, #(16 * 2)]
    ldp x6, x7, [sp, #(16 * 3)]
    ldp x8, x9, [sp, #(16 * 4)]
    ldp x10, x11, [sp, #(16 * 5)]
    ldp x12, x13, [sp, #(16 * 6)]
    ldp x14, x15, [sp, #(16 * 7)]
    ldp x16, x17, [sp, #(16 * 8)]
    ldp x18, x19, [sp, #(16 * 9)]
    ldp x20, x21, [sp, #(16 * 10)]
    ldp x22, x23, [sp, #(16 * 11)]
    ldp x24, x25, [sp, #(16 * 12)]
    ldp x26, x27, [sp, #(16 * 13)]
    ldp x28, x29, [sp, #(16 * 14)]
    ldp lr, xzr, [sp, #(16 * 15)]
    add sp, sp, #(16 * 16)
    eret

.balign 0x80
irq_kernel:
    ldr x0, =halt
    br x0

.balign 0x80
fiq_kernel:
    ldr x0, =halt
    br x0

.balign 0x80
serror_kernel:
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_userspace:
    ldr x0, =halt
    br x0

.balign 0x80
irq_userspace:
    ldr x0, =halt
    br x0

.balign 0x80
fiq_userspace:
    // FIQ
    ldr x0, =halt
    br x0

.balign 0x80
serror_userspace:
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_userspace_aarch32:
    ldr x0, =halt
    br x0

.balign 0x80
irq_userspace_aarch32:
    ldr x0, =halt
    br x0

.balign 0x80
fiq_userspace_aarch32:
    ldr x0, =halt
    br x0

.balign 0x80
serror_userspace_aarch32:
    ldr x0, =halt
    br x0

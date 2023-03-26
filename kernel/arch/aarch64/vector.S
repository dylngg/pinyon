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
    adr x0, kernel_vtable
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
kernel_vtable:
.balign 0x80
synchronous_wrap_with_same_el_sp0:
    // Synchronous
    ldr x0, =halt
    br x0

.balign 0x80
irq_wrap_with_same_el_sp0:
    // IRQ
    ldr x0, =halt
    br x0

.balign 0x80
fiq_wrap_with_same_el_sp0:
    // FIQ
    ldr x0, =halt
    br x0

.balign 0x80
serror_wrap_with_same_el_sp0:
    // SError
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_wrap_with_same_el:
    // Synchronous
    ldr x0, =halt
    br x0

.balign 0x80
irq_wrap_with_same_el:
    // IRQ
    ldr x0, =halt
    br x0

.balign 0x80
fiq_wrap_with_same_el:
    // FIQ
    ldr x0, =halt
    br x0

.balign 0x80
serror_wrap_with_same_el:
    // SError
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_wrap_with_lower_el:
    // Synchronous
    ldr x0, =halt
    br x0

.balign 0x80
irq_wrap_with_lower_el:
    // IRQ
    ldr x0, =halt
    br x0

.balign 0x80
fiq_wrap_with_lower_el:
    // FIQ
    ldr x0, =halt
    br x0

.balign 0x80
serror_wrap_with_lower_el:
    // SError
    ldr x0, =halt
    br x0

.balign 0x80
synchronous_wrap_with_lower_el_aarch32:
    // Synchronous
    ldr x0, =halt
    br x0

.balign 0x80
irq_wrap_with_lower_el_aarch32:
    // IRQ
    ldr x0, =halt
    br x0

.balign 0x80
fiq_wrap_with_lower_el_aarch32:
    // FIQ
    ldr x0, =halt
    br x0

.balign 0x80
serror_wrap_with_lower_el_aarch32:
    // SError
    ldr x0, =halt
    br x0

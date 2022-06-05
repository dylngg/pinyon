.section .text
.global setup_vtable
.global setup_stacks
.global enable_irq

/* FIXME? These should just be macros? We don't change them anywhere. */
super_sp = 0xb000
irq_sp = 0x3000
abort_sp = 0x2000
undef_sp = 0x1000

setup_vtable:
    ldr r0, =vtable
    mov r1, #0         /* Load vtable address */

    /*
     * First load the 8 words of r0 (vtable) into corresponding r2-r9 registers;
     * Here, the ia is the magic: after loading a particular register, the
     * address for the next register increments. The result is a cascading 8
     * word load of the contents of r0 into 8 registers.
     */
    ldmia r0!, {r2-r9}
    /*
     * Now do the opposite, take those 8 registers (vtable entries) and write
     * them to r1, the location of the vector table using the same tricks.
     */
    stmia r1!, {r2-r9}
    /*
     * r0 is now pointing to immediately _after_ the vector table thanks to the
     * magic ! char on r0!, which says to store the address after the last load
     * into r0.
     *
     * Now we load and store our branch-to-abs-branch stuff to the piece of
     * memory after the vector table. Same magic with r1!.
     */
    ldmia r0!, {r2-r8}
    stmia r1!, {r2-r8}

setup_stacks:
    /* Each mode, including IRQ mode, has their own stack. We'll use this to save
     * registers in the IRQ handler.
     *
     * WARNING! If stacks are changed, you will need to modify bootup.S and linker.ld
     */
    mov sp, #super_sp   /* Set up stack (8KiB); after IRQ and abort stacks. */
    cps #0x12           /* Enter IRQ mode */
    mov sp, #irq_sp      /* Small IRQ stack of 4KiB; we don't do much processing */
    cps #0x17           /* Enter Abort mode */
    mov sp, #abort_sp    /* Small Abort mode stack of 4KiB */
    cps #0x1b           /* Enter Undefined instruction mode */
    mov sp, #undef_sp    /* Small Undefined mode stack of ~4KiB (minus vector table size) */
    cps #0x13           /* Go back to supervisor mode */
    bx lr

enable_irq:
    cpsie i             /* Enable IRQ interrupts */
    bx lr

    /*
     * Vector tables on arm are basically a table of 4 byte code, so we want
     * to setup a table here where we branch to particular C function. The
     * positions of each function needs to be relative to the vector table
     * location (at 0) due to us using -fpic (position independent code), so
     * our solution is to branch to another branch set up relative to the
     * table.
     */
vtable:
    ldr pc, reset_offset
    ldr pc, undefined_instruction_offset
    ldr pc, swi_offset
    ldr pc, prefetch_abort_offset
    ldr pc, data_abort_offset
    nop                     /* hypervisor trap; not used */
    ldr pc, irq_offset
    ldr pc, fiq_offset

reset_offset:                   .word halt
undefined_instruction_offset:   .word undefined_instruction_wrap
swi_offset:                     .word swi_wrap
prefetch_abort_offset:          .word prefetch_abort_handler
data_abort_offset:              .word data_abort_handler
irq_offset:                     .word irq_wrap
fiq_offset:                     .word fast_irq_handler

undefined_instruction_wrap:
    /*
     * We hit an undefined instruction and are switched to undefined mode. The
     * processor has saved the old PC into LR, and the old LR is saved into the
     * old mode's LR. The SP is this mode's SP. The old CPSR is also saved into
     * this mode's SPSR.
     */
    mrs r0, spsr            /* arg1 (SPSR) */
    mov r1, lr              /* arg2 (old PC) */
    bic r3, r0, #0x7fffffe0 /* shadow mode */
    msr cpsr, r3
    mov r2, lr              /* arg3 (old LR) */
    cps #0x1b               /* undefined mode */
    bl undefined_instruction_handler
    b halt

irq_wrap:
    /*
     * We are interrupting a task in IRQ mode. The processor has saved the old
     * PC into LR, and the old LR is saved in the old mode's LR. The old CPSR
     * is also saved into this mode's SPSR. The SP is this mode's SP (whatever
     * it last was). Also, interrupts are disabled.
     *
     * See also B1.8.3 in ARMv7 Reference Manual
     */
    sub lr, #4              /* LR points to next instruction */
    srsdb sp!, #0x13        /* Save LR and SPSR onto supervisor stack */
    cpsid i, #0x13          /* Switch to supervisor mode, clobbering LR but retrieving kernel SP */
    push {r0-r3, r12}
    bl irq_handler
    pop {r0-r3, r12}
    rfeia sp!               /* Return to LR, restore SPRS into CPRS (e.g. returns back to old mode) */

swi_wrap:
    /*
     * We are responding to a task syscall (called from user mode). The
     * processor has saved the old PC into LR, and the old LR is saved in the
     * old mode's LR. The old CPSR is also saved into this mode's SPSR, so
     * we'll have to save that upon restoration. Also, interrupts are disabled.
     *
     * Notably, we do not have to do as much register saving as the IRQ handler
     * as this is a voluntary call from a task; we can assume they are
     * following the C ABI calling conventions.
     */
    srsdb sp!, #0x13        /* Save LR and SPSR */
    push {r12}              /* r12 caller saved register (r0-r3 are syscall args) */
    cpsie i                 /* Re-enable interrupts; allow for interrupt nesting to occur */
    bl software_interrupt_handler
    pop {r12}
    mov r1, #0              /* r0 is used as return value; we shouldn't leak for the rest */
    mov r2, #0
    mov r3, #0
    rfeia sp!               /* Return to LR, restore SPRS into CPRS (e.g. returns back to old mode) */

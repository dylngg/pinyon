.section .text
.global setup_vtable
.global enable_irq

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

    /* Each mode, including IRQ mode, has their own stack. We'll use this to save
     * registers in the IRQ handler. */
    cps #0x12           /* Enter IRQ mode*/
    mov sp, #0x1000     /* Small IRQ stack of 4KiB, this maybe a problem in the future... */
    cps #0x13           /* Go back to supervisor mode */
    bx lr

enable_irq:
    cpsie i             /* Enable IRQ interrupts */
    dmb
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
    ldr pc, reset_b
    ldr pc, undef_instr_b
    ldr pc, swi_b
    ldr pc, prefch_abrt_b
    ldr pc, data_abrt_b
    nop                     /* hypervisor trap; not used */
    ldr pc, irq_b
    ldr pc, fiq_b

    /* Branch-to-another-abs-branch part */
reset_b:        .word halt
undef_instr_b:  .word undefined_instruction_handler
swi_b:          .word swi_b_wrap
prefch_abrt_b:  .word prefetch_abort_handler
data_abrt_b:    .word data_abort_handler
irq_b:          .word irq_b_wrap
fiq_b:          .word fast_irq_handler

irq_b_wrap:
    /*
     * We are interrupting a task in IRQ mode. The processor has saved the old
     * PC into LR, and the old LR is saved in the old mode's LR. The old CPSR
     * is also saved into this mode's SPSR, so we'll have to save that upon
     * restoration. Also, interrupts are disabled.
     *
     * It should be noted that each mode also has their own stack, but this
     * feature is not presently used.
     *
     * The following slide deck found on the internet is extremely useful:
     * http://s3-us-west-2.amazonaws.com/valpont/uploads/20160326012043/Exception_handling.pdf
     *
     * See also B1.8.3 in ARMv7 Reference Manual
     */
    sub lr, lr, #4          /* LR points to next instruction; undo that quirk */
    srsdb #0x13!            /* Save SPRS and LR on the supervisor stack */
    cps #0x13               /* Switch to supervisor mode */
    push {r0-r4, r12, lr}   /* Save caller registers for irq_handler call */
    and r4, sp, #4          /* Align the stack */
    sub sp, sp, r4
    bl irq_handler
    add sp, sp, r4          /* Undo stack alignment */
    pop {r0-r4, r12, lr}
    cpsie i                 /* Reenable interrupts so we don't get stuck in that task */
    rfeia sp!               /* Return to LR, restore SPRS into CPRS (e.g. returns back to old mode) */

swi_b_wrap:
    /*
     * At this point we are in supervisor mode, so SP is our supervisor stack
     * and LR is our supervisor link register*, which points to after the SWI
     * instruction. If we want to get the number assicated with the SWI, we
     * basically have to load the SWI instruction (LR-4) into a register and
     * mask off the last 8 bits (the opcode, bits 31-24). Also, interrupts are
     * disabled.
     *
     * Notably, we do not have to do as much register saving as the IRQ handler
     * as this is a voluntary call from a task; we can assume they are
     * following the C ABI calling conventions.
     *
     * *Of note is the fact that tasks currently live in supervisor mode; this
     *  means the old LR is clobbered by the saving of LR into the calling
     *  mode's LR register. So, LR must be preserved by tasks before a SWI.
     *
     * The following docs were useful:
     * https://heyrick.eu/assembler/swi.html
     * https://iitd-plos.github.io/col718/ref/arm-instructionset.pdf
     */
    // FIXME: Switch back to old CPSR; we can get away with it right now because we never drop privs
    push {r0-r4, r12, lr}   /* We're going to B to C code, so we need to save caller registers */
    cpsie i                 /* Re-enable interrupts; allow for interrupt nesting to occur */
    and r4, sp, #4          /* Align the stack */
    sub sp, sp, r4
    mov r2, r1              /* Copy user args into user defined arg */
    mov r1, r0
    ldr r0, [lr, #-4]       /* Get SWI instruction */
    bic r0, r0, #0xFF000000 /* Mask off last 8 bits to get syscall id */
    bl software_interrupt_handler
    add sp, sp, r4          /* Undo stack alignment */
    pop {r0-r4, r12, lr}
    bx lr

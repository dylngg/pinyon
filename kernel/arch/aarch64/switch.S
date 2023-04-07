.section .text

.global task_switch
.global task_kernel_return

/*
 * When context switching we need to:
 *   a) Save the old task's general registers (x0-x28) into the Register struct
        pointed by x0 (x0 will be saved by caller conventions).
 *   b) Save the old task's LR, SP from user mode, LR, SP from supervisor mode,
 *      and their PSTATE, since we're going to clobber those banked registers
 *      when we switch. The SPSR is naturally going to clobbered (it happens
 *      whenever we change modes), so interrupt handlers need to save and
 *      restore those.
 *   c) Restore the same things in reverse
 *
 * Note that we set the supervisor SP to the task's kernel SP when a task
 * starts initially. This guarantees that the SP in supervisor mode while the
 * task is running is the task's kernel SP.
 */

_task_save:
    /*
     * Saves the current context into the given registers (x0) for a context
     * switch, returning false. When a context switch occurs back to this
     * Task, _task_save is returned from once again with a true value.
     */
    /* Note that x2 and x3 are saved before calling, x0 is the Register* to save to */
    mov x2, #0
    mrs x3, currentel           /* Set the current EL in the SPSR */
    cmp x3, #0
    beq _task_save_el0
    add x3, x3, #1              /* If it is EL1 change to ELh */
_task_save_el0:
    orr x2, x2, x3
    mrs x3, daif                /* Set the exception levels in the SPSR */
    orr x2, x2, x3
    mrs x3, nzcv                /* Set the condition flags in the SPSR */
    orr x2, x2, x3

    ldr x3, =_task_return_again /* PC for context switch back */
    stp x2, x3, [x0]            /* Save the newly created SPSR and PC */
    cmp x1, #0
    bne _task_save_registers
    mrs x2, sp_el0
    str x2, [x0, #16]           /* Save user stack */
_task_save_registers:
    mov x2, sp
    stp x2, lr, [x0, #24]       /* Save SP and LR from current mode */
    stp x28, x29, [x0, #40 + (16 * 0)]
    stp x26, x27, [x0, #40 + (16 * 1)]
    stp x24, x25, [x0, #40 + (16 * 2)]
    stp x22, x23, [x0, #40 + (16 * 3)]
    stp x20, x21, [x0, #40 + (16 * 4)]
    stp x18, x19, [x0, #40 + (16 * 5)]
    stp x16, x17, [x0, #40 + (16 * 6)]
    stp x14, x15, [x0, #40 + (16 * 7)]
    stp x12, x13, [x0, #40 + (16 * 8)]
    stp x10, x11, [x0, #40 + (16 * 9)]
    stp x8, x9, [x0, #40 + (16 * 10)]
    stp x6, x7, [x0, #40 + (16 * 11)]
    stp x4, x5, [x0, #40 + (16 * 12)]
    stp x2, x3, [x0, #40 + (16 * 13)]
    stp x0, x1, [x0, #40 + (16 * 14)]
    mov x0, #0
    b _task_return
_task_return_again:
    mov x0, #1
_task_return:
    ret

task_switch:
    /*
     * This is exported as:
     * void task_switch(Registers* to_save_registers, bool is_kernel_task_save, const Registers* stored_registers, bool is_kernel_task_stored);
     *
     * Saves the current task into the first given registers (x0) if not NULL,
     * and switches to the second given registers.
     */
    cmp x0, #0                      /* If NULL to_save_registers */
    beq _task_start

    sub sp, sp, #32
    stp x2, x3, [sp]
    stp lr, xzr, [sp, #16]
    bl _task_save
    ldp x2, x3, [sp]
    ldp lr, xzr, [sp, #16]
    add sp, sp, #32
    cmp x0, #0                      /* Check if returning from context switch */
    beq _task_start                 /* return if coming from context switch */
    ret

_task_start:
    ldp x0, x1, [x2]                /* Restore the CPSR and PC */
    msr spsr_el1, x0
    msr elr_el1, x1
    cmp x3, #0
    bne _task_start_registers
    ldr x0, [x2, #16]               /* Restore SP in user mode if not is_kernel_task_stored */
    msr sp_el0, x0
_task_start_registers:
    ldp x1, lr, [x2, #24]           /* Restore SP and LR of current mode; if we are starting a new task this is their supervisor SP and LR */
    mov sp, x1
    add x0, x2, #40                 /* Copy x2 (stored_registers) into x0 for usage after clobbering */
    ldp x28, x29, [x0, #(16 * 0)]
    ldp x26, x27, [x0, #(16 * 1)]
    ldp x24, x25, [x0, #(16 * 2)]
    ldp x22, x23, [x0, #(16 * 3)]
    ldp x20, x21, [x0, #(16 * 4)]
    ldp x18, x19, [x0, #(16 * 5)]
    ldp x16, x17, [x0, #(16 * 6)]
    ldp x14, x15, [x0, #(16 * 7)]
    ldp x12, x13, [x0, #(16 * 8)]
    ldp x10, x11, [x0, #(16 * 9)]
    ldp x8, x9, [x0, #(16 * 10)]
    ldp x6, x7, [x0, #(16 * 11)]
    ldp x4, x5, [x0, #(16 * 12)]
    ldp x2, x3, [x0, #(16 * 13)]
    ldp x0, x1, [x0, #(16 * 14)]
    eret

task_kernel_return:
    // FIXME: We should panic here, since if somehow the kernel LR is returned
    //        to, things have gone badly wrong
    b halt

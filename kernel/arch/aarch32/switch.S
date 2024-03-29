.section .text

.global task_switch
.global task_kernel_return

/*
 * When context switching we need to:
 *   a) Save the old task's general registers (r0-r12) into the Register struct
        pointed by r0 (r0 will be saved by caller conventions).
 *   b) Save the old task's LR, SP from user mode, LR, SP from supervisor mode,
 *      and their CPSR, since we're going to clobber those banked registers
 *      when we switch. The SPSR is naturally going to clobbered (it happens
 *      whenever we change modes), so interrupt handlers need to save and
 *      restore those.
 *   c) Restore the same things in reverse
 *
 * Note that we set the supervisor SP to the task's kernel SP when a task
 * starts initially. This guarantees that the SP in supervisor mode while the
 * task is running is the task's kernel SP.
 */

task_save:
    /*
     * This is exported as:
     * void task_save(Registers* to_save_registers, bool is_kernel_task);
     *
     * Saves the current context into the given registers (r0) for a context
     * switch, returning false. When a context switch occurs back to this
     * Task, task_save() is returned from once again with a true value.
     */
    /* Note: It is also ok to use r1-r3 because they are caller saved */
    mrs r2, cpsr                /* Save CPSR into r2 */
    str r2, [r0]
    add r0, #4
    cmp r1, #0
    stmeq r0, {sp, lr}^       /* Save SP and LR from user mode if not is_kernel_task */
    add r0, #8                  /* cannot use ! in prev stmia */
    stm r0!, {sp, lr}         /* Save SP and LR from current mode */
    stm r0!, {r0-r12}         /* Save general registers */
    ldr r2, =_task_return_again /* PC for context switch back */
    str r2, [r0]
    mov r0, #0
    b _task_return
_task_return_again:
    mov r0, #1
_task_return:
    bx lr

task_switch:
    /*
     * This is exported as:
     * void task_switch(Registers* to_save_registers, bool is_kernel_task_save, const Registers* stored_registers, bool is_kernel_task_stored);
     *
     * Saves the current task into the first given registers (r0) if not NULL,
     * and switches to the second given registers.
     */
    cmp r0, #0                  /* If NULL to_save_registers */
    beq _task_start

    push {r0-r3, r12, lr}
    bl task_save
    cmp r0, #0                  /* Check if returning from context switch */
    pop {r0-r3, r12, lr}
    bxne lr                     /* return if coming from context switch */

_task_start:
    ldr r1, [r2]                /* Restore the CPSR into the SPSR */
    msr spsr, r1
    add r2, #4
    cmp r3, #0
    ldmeq r2, {sp, lr}^       /* Restore SP and LR in user mode if not is_kernel_task_stored */
    add r2, #8
    ldm r2!, {sp, lr}         /* Restore SP and LR of current mode; if we are starting a new task this is their supervisor SP and LR */
    ldm r2, {r0-r12, pc}^     /* Go to PC; SPSR is copied into CPSR */

task_kernel_return:
    // FIXME: We should panic here, since if somehow the kernel LR is returned
    //        to, things have gone badly wrong
    b halt

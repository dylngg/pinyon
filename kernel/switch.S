.section .text

.global task_save
.global task_start
.global task_resume

/*
 * When context switching we need to:
 *   a) Save the old task's general registers (r0-r12) on the task's stack.
 *   b) Save the old task's lr, sp, and cpsr. Depending on how schedule()
 *      gets called, the way we get this is different. If we're called from
 *      the IRQ handler (which enters in IRQ mode and switches to
 *      supervisor mode), then we have to drop back to IRQ mode to obtain
 *      them from their shadow registers. On the other hand, if this happens
 *      in a yield() syscall and we're called from the SWI handler, then we
 *      don't have to do that since they should be in our shadow registers
 *      already.
 *   c) Restore all of the new task's registers (general and stack)
 *   d) Start the task where it left off
 *
 */

task_save:
    /*
     * This is exported as:
     * void task_save(u32 *old_sp_ptr);
     *
     * We basically just save the running task's stack pointer and registers
     * here.
     */
    push {r0-r12}
    str sp, [r0]        /* Store sp into old sp addr */
    bx lr

task_start:
    /*
     * This is exported as:
     * void task_start(u32 new_pc, u32 new_sp)
     *
     * When we start a new task that has not previously been run, we do not
     * return through the IRQ or SWI handler, but instead just override the
     * program counter and stack and start that way.
     */
    ldr lr, =halt       /* If the task returns, stop. Could be improved. */
    mov sp, r1          /* Set new stack pointer */
    cpsie i             /* Reenable interrupts if they were disabled */
    mov pc, r0          /* Set new program counter */

task_resume:
    /*
     * This is exported as:
     * void context_switch(u32 new_sp);
     *
     * To resume a task we should set the task's stack pointer, pop their old
     * register values off that stack and finally return the way they came (IRQ
     * or SWI). This is very much implicit since that return information and path
     * is implicitly stored on the task's stack when we task_save.
     *
     * Ideally this should set the correct CPSR, but since all tasks run in
     * supervisor mode this is not necessary. (I think?)
     */
    mov sp, r0          /* Copy new stack pointer address into stack pointer*/
    pop {r0-r12}        /* Restore new task registers */
    bx lr               /* Return to lr (schedule) */
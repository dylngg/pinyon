.section .text
.global syscall0
.global syscall1
.global syscall2

/*
 * Note: We must save the LRs here as a side-effect of tasks always living in
 *       supervisor mode. See swi_wrap in vector.S for details.
 */

syscall0:
syscall1:
syscall2:
    push {lr}
    swi 0
    pop {lr}
    bx lr

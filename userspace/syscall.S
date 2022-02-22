.section .text
.global syscall_yield
.global syscall_sleep
.global syscall_read
.global syscall_write
.global syscall_sbrk
.global syscall_uptime
.global syscall_cputime

/*
 * Note: We must save the LRs here as a side-effect of tasks always living in
 *       supervisor mode. See swi_wrap in vector.S for details.
 */

syscall_yield:
    push {lr}
    swi 0
    pop {lr}
    bx lr

syscall_sleep:
    push {lr}
    swi 1
    pop {lr}
    bx lr

syscall_read:
    push {lr}
    swi 2
    pop {lr}
    bx lr

syscall_write:
    push {lr}
    swi 3
    pop {lr}
    bx lr

syscall_sbrk:
    push {lr}
    swi 4
    pop {lr}
    bx lr

syscall_uptime:
    push {lr}
    swi 5
    pop {lr}
    bx lr

syscall_cputime:
    push {lr}
    swi 6
    pop {lr}
    bx lr

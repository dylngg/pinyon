.section .text
.global syscall_yield
.global syscall_sleep
.global syscall_readline
.global syscall_write
.global syscall_heap_reserve
.global syscall_heap_incr
.global syscall_uptime
.global syscall_cputime

/*
 * I'm not sure if this is a standard thing, or a bug, but the link
 * register after a syscall points to the instruction after the syscall
 * (which _is_ correct when we're in the swi_handler but not after we
 * return from that handler), rather than this function's caller...
 */
// FIXME: Fix lr being clobbered after a swi (bug?!)

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

syscall_readline:
    push {lr}
    swi 2
    pop {lr}
    bx lr

syscall_write:
    push {lr}
    swi 3
    pop {lr}
    bx lr

syscall_heap_reserve:
    push {lr}
    swi 4
    pop {lr}
    bx lr

syscall_heap_incr:
    push {lr}
    swi 5
    pop {lr}
    bx lr

syscall_uptime:
    push {lr}
    swi 6
    pop {lr}
    bx lr

syscall_cputime:
    push {lr}
    swi 7
    pop {lr}
    bx lr

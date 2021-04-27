.section .text
.global syscall_yield
.global syscall_sleep

syscall_yield:
    // I'm not sure if this is a standard thing, or a bug, but the link
    // register after a syscall points to the instruction after the syscall
    // (which _is_ correct when we're in the swi_handler but not after we
    // return from that handler), rather than this function's caller...
    // FIXME: Fix lr being clobbered after a swi (bug?!)
    push {lr}
    swi 0
    pop {lr}
    bx lr

syscall_sleep:
    push {lr}
    swi 1
    pop {lr}
    bx lr

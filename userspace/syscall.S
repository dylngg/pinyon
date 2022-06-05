.section .text
.global syscall0
.global syscall1
.global syscall2

syscall0:
syscall1:
syscall2:
    swi 0
    bx lr

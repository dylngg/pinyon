.text
.global syscall0
.global syscall1
.global syscall2
.global syscall3

syscall0:
syscall1:
syscall2:
syscall3:
    svc 0
    ret
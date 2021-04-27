#pragma once

// See syscall.S
extern "C" {
void syscall_yield();
void syscall_sleep(u32 secs);
}
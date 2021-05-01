#pragma once

#include <pine/printf.hpp>
#include <pine/string.hpp>

// See syscall.S
extern "C" {
void syscall_yield();
void syscall_sleep(u32 secs);
void syscall_readline(char* buf, u32 bytes);
void syscall_write(char* buf, u32 bytes);
}

void readline(char* buf, u32 bytes);

void yield();

void sleep(u32 secs);

void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

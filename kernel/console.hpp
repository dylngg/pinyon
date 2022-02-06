#pragma once

#include <pine/types.hpp>

/*
 * Kernel functions that poll. This is meant for logging, not for userspace
 * usage (use read/write for that).
 */
void console_readline(char*, size_t);

void console(const char*);

void console(const char*, size_t);

void consoleln(const char*);

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));
#pragma once

#include <pine/types.hpp>

/*
 * Logging functions for the kernel; these poll the UART, instead of using IRQs
 * since we want the message to be pushed out before we return.
 */
void console(const char*);

void console(const char*, size_t);

void consoleln(const char*);

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));
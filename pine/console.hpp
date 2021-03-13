#pragma once
#include <pine/string.hpp>

// Magic number I found at https://jsandler18.github.io/explanations/kernel_c.html
// Note: This is specific to the Raspberry Pi 2/3
#define UART0_BASE 0x3F201000

void console_put(char ch);

void console(const String& message);

void consoleln(const String& message);

void consolef(const String& fmt, ...);
#pragma once

void panic(const char* message);
void panicf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void panic_if(bool condition, const char* message);

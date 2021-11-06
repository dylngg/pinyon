#pragma once

void panic(const char* message);
void panicf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#define PANIC_IF(cond, message) \
    if ((cond)) \
        panicf("%s:%d %s\t\"%s\" condition failed. %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond, message);

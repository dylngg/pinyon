#pragma once

void panic(const char* message);
void panicf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#define PANIC_MESSAGE_IF(cond, message) \
    if ((cond))                         \
        panicf("%s:%d %s\t\"%s\" condition failed. %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond, message);
#define PANIC_IF(cond) \
    if ((cond))        \
        panicf("%s:%d %s\t\"%s\" condition failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #cond);

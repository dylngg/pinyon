#pragma once

#ifdef AARCH64
#include "arch/aarch64/panic.hpp"
#elif AARCH32
#include "arch/aarch32/panic.hpp"
#else
#error Architecture not defined
#endif

#define PANIC_MESSAGE_IF(cond, message) \
    if ((cond))                         \
        panic(__FILE__, ":", __LINE__, " ", __PRETTY_FUNCTION__, "\t\"", #cond, "\" condition failed.", message);
#define PANIC_IF(cond) \
    if ((cond))        \
        PANIC_MESSAGE_IF((cond), "");

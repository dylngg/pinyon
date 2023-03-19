#pragma once
#ifdef AARCH64
#elif AARCH32
#include "arch/aarch32/processor.hpp"
#else
#error Architecture not defined
#endif
struct InterruptDisabler;

struct InterruptsDisabledTag {
    InterruptsDisabledTag(const InterruptDisabler&) {};
    static InterruptsDisabledTag promise() { return InterruptsDisabledTag {}; };

private:
    InterruptsDisabledTag() = default;
};

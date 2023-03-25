#pragma once

struct InterruptDisabler;

struct InterruptsDisabledTag {
    InterruptsDisabledTag(const InterruptDisabler&) {};
    static InterruptsDisabledTag promise() { return InterruptsDisabledTag {}; };

private:
    InterruptsDisabledTag() = default;
};

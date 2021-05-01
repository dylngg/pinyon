#pragma once

// Forces a data memory barrier on creation and destroy. This is only needed
// when the underlying virtual memory map is not strongly ordered.
class MemoryBarrier {
public:
    MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }

    ~MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }

    static void sync() __attribute__((always_inline))
    {
        asm volatile("dmb");
    }
};

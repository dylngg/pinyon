#pragma once

namespace pine {

// Forces a memory barrier on creation and destroy.
// FIXME: Determine whether oshld is right!
class MemoryBarrier {
public:
    MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb oshld");
    }

    ~MemoryBarrier() __attribute__((always_inline))
    {
        asm volatile("dmb oshld");
    }

    static void sync() __attribute__((always_inline))
    {
        asm volatile("dmb oshld");
    }
};

// Forces a data memory barrier on creation and destroy.
class DataBarrier {
public:
    DataBarrier() __attribute__((always_inline))
    {
        asm volatile("dsb oshld");
    }

    ~DataBarrier() __attribute__((always_inline))
    {
        asm volatile("dsb oshld");
    }

    static void sync() __attribute__((always_inline))
    {
        asm volatile("dsb oshld");
    }
};

}

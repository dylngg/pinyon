#pragma once
#include <pine/types.hpp>

enum class ProcessorMode : u32 {
    User = 0b10000,
    FIQ = 0b10001,
    IRQ = 0b10010,
    Supervisor = 0b10011,
    Monitor = 0b10110,
    Abort = 0b10111,
    Hypervisor = 0b11010,
    Undefined = 0b11011,
    System = 0b11111,
};

struct CPSR {
    explicit CPSR(ProcessorMode mode)
        : m(mode)
        , t(0)
        , f(0)
        , i(0)
        , a(0)
        , e(0)
        , it_1(0)
        , ge(0)
        , _(0)
        , j(0)
        , it_2(0)
        , q(0)
        , v(0)
        , c(0)
        , z(0)
        , n(0) {};

    static CPSR cpsr()
    {
        CPSR cpsr {};
        asm volatile("mrs %0, cpsr" :"=r"(cpsr));
        return cpsr;
    }
    static CPSR spsr()
    {
        CPSR spsr {};
        asm volatile("mrs %0, spsr" :"=r"(spsr));
        return spsr;
    }

    [[nodiscard]] bool are_interrupts_disabled() const
    {
        return f || i || a;
    }
    [[nodiscard]] bool in_privileged_mode() const
    {
        return m != ProcessorMode::User && m != ProcessorMode::System;
    }
    [[nodiscard]] ProcessorMode mode() const
    {
        return m;
    }

    void disable_interrupts()
    {
        f = 1;
        i = 1;
        a = 1;
    }
    void set_as_cpsr()
    {
        asm volatile("msr cpsr, %0" ::"r"(*this));
    }

    ProcessorMode m: 5;     // 0-4: Mode (User, FIQ, IRQ, Supervisor, etc...)
    u32 t: 1;               // 5: Thumb execution bit
    u32 f: 1;               // 6: FIQ mask bit
    u32 i: 1;               // 7: IRQ mask bit
    u32 a: 1;               // 8: Asynchronous abort mask bit
    u32 e: 1;               // 9: Endianness (0: little, 1: big)
    u32 it_1: 6;            // 10-15: First 6 bits of IT exception state for IT instruction
    u32 ge: 4;              // 16-19: Greater than or equal flags or SIMD parallel add/sub
    u32 _: 4;               // 20-23: Reserved. RAZ/SBZP
    u32 j: 1;               // 24: Jazelle bit
    u32 it_2: 2;            // 25-26: Last 2 bits of IT exception state for IT instruction
    u32 q: 1;               // 27: Cumulative saturation bit
    u32 v: 1;               // 28: Overflow condition flag
    u32 c: 1;               // 29: Carry condition flag
    u32 z: 1;               // 30: Zero condition flag
    u32 n: 1;               // 31: Negative condition flag

private:
    explicit CPSR() = default;  // used by static methods to avoid proper initialization
};

#pragma once
#include <pine/types.hpp>
#include <pine/print.hpp>
#include <pine/bit.hpp>

struct InterruptDisabler {
    InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("cpsid i");
    }

    ~InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("cpsie i");
    }
};

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

void print_with(pine::Printer& printer, const ProcessorMode& mode);

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

    static CPSR from_data(PtrData cpsr_as_u32)
    {
        // For some reason reinterpret_cast doesn't
        return pine::bit_cast<CPSR>(cpsr_as_u32);
    }
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

    friend void print_with(pine::Printer& printer, const CPSR& cpsr);

private:
    explicit CPSR() = default;  // used by static methods to avoid proper initialization

    template <typename To, typename From>
    friend constexpr To pine::bit_cast(From);
};

struct Registers {
    explicit Registers(u32 user_sp, u32 kernel_sp, u32 user_pc, PtrData stop_addr, ProcessorMode user_mode)
        : cpsr(user_mode)
        , user_sp(user_sp)
        , user_lr(stop_addr)
        , kernel_sp(kernel_sp)
        , kernel_lr(stop_addr)
        , r0(0)
        , r1(0)
        , r2(0)
        , r3(0)
        , r4(0)
        , r5(0)
        , r6(0)
        , r7(0)
        , r8(0)
        , r9(0)
        , r10(0)
        , r11(0)
        , r12(0)
        , pc(user_pc) {};

    bool is_kernel_registers() const { return user_sp == kernel_sp; };

    CPSR cpsr;  // The CPSR to return to (either user or kernel, depending on if starting a task)
    u32 user_sp;
    u32 user_lr;
    u32 kernel_sp;
    u32 kernel_lr;
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    u32 r11;
    u32 r12;
    u32 pc;     // The PC to return to (either user or kernel, depending on if starting a task)
};

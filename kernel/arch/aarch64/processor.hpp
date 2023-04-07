#pragma once
#include <pine/bit.hpp>
#include <pine/print.hpp>
#include <pine/types.hpp>
#include <pine/syscall.hpp>

struct InterruptDisabler {
    InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("msr daifset, #0b0011");
    }

    ~InterruptDisabler() __attribute__((always_inline))
    {
        asm volatile("msr daifclr, #0b0011");
    }

    InterruptDisabler(const InterruptDisabler&) = delete;
    InterruptDisabler(InterruptDisabler&&) = delete;

    static PtrData status();
};

// See https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/ESR-EL1--Exception-Syndrome-Register--EL1-
enum class ExceptionClass : u64 {
    UnknownReason = 0x0,
    SIMDException = 0x7,
    SVC = 0x15,
    InstructionAbortEL0 = 0x20,
    InstructionAbort = 0x21,
    DataAbortEL0 = 0x24,
    DataAbort = 0x25,
    FPException = 0x2c,
    SError = 0x2f,
};

struct ESR_EL1 {
    u64 iss : 25;           // 0-24: Instruction Specific Syndrome
    u64 il : 1;             // 25: Instruction Length
    ExceptionClass ec : 6;  // 26-31: Exception Class
    u64 iss2 : 5;           // 32-36: When FEAT_LS64 is implemented, holds register specifier, Xs
    u64 reserved : 27;      // 37-63: Reserved bits
};

static_assert(sizeof(ESR_EL1) == 8);

enum class ProcessorMode : u64 {
    EL0 = 0b0000,
    EL1t = 0b0100,
    EL1h = 0b0101,
};

void print_with(pine::Printer& printer, const ProcessorMode& mode);

struct SPSR_EL1 {
    explicit SPSR_EL1(ProcessorMode mode)
        : m(mode)
        , m2(0)
        , _(0)
        , f(0)
        , i(0)
        , a(0)
        , d(0)
        , _1(0)
        , _2(0)
        , il(0)
        , ss(0)
        , _3(0)
        , _4(0)
        , v(0)
        , c(0)
        , z(0)
        , n(0)
        , _5(0) {};

    static SPSR_EL1 from_data(PtrData spsr)
    {
        return pine::bit_cast<SPSR_EL1>(spsr);
    }
    static SPSR_EL1 spsr()
    {
        SPSR_EL1 spsr {};
        asm volatile("mrs %0, spsr_el1" :"=r"(spsr));
        return spsr;
    }

    friend void print_with(pine::Printer& printer, const SPSR_EL1& cpsr);

    ProcessorMode m : 4;    // 0-3: AArch64 Exception level and selected Stack Pointer
    u64 m2 : 1;             // 4: Execution state
    u64 _ : 1;              // 5: Reserved
    u64 f : 1;              // 6: FIQ interrupt mask
    u64 i : 1;              // 7: IRQ interrupt mask
    u64 a : 1;              // 8: SError interrupt mask
    u64 d : 1;              // 9: Debug interrupt mask
    u64 _1 : 3;             // 10-12: Other
    u64 _2 : 7;             // 13-19: Reserved
    u64 il : 1;             // 20: Illegal Execution state
    u64 ss : 1;             // 21: Software Step
    u64 _3 : 4;             // 22-25: Other
    u64 _4 : 2;             // 26-27: Reserved
    u64 v : 1;              // 28: Overflow Condition flag
    u64 c : 1;              // 29: Carry Condition flag
    u64 z : 1;              // 30: Zero Condition flag
    u64 n : 1;              // 31: Negative Condition bit
    u64 _5 : 32;            // 32-63: Reserved

private:
    explicit SPSR_EL1() = default;  // used by static methods to avoid proper initialization

    template <typename To, typename From>
    friend constexpr To pine::bit_cast(From);
};

static_assert(sizeof(SPSR_EL1) == 8);

struct ExceptionSavedRegisters {
    u64 xn[30];
    u64 lr;
    u64 zero;

    friend void print_with(pine::Printer& printer, const ExceptionSavedRegisters& registers);
};

struct Registers {
    explicit Registers(PtrData user_sp, PtrData kernel_sp, PtrData user_pc, PtrData stop_addr, PrivilegeLevel level)
        : cpsr(level == PrivilegeLevel::Kernel ? ProcessorMode::EL1h : ProcessorMode::EL0)
        , pc(user_pc)
        , user_sp(user_sp)
        , kernel_sp(kernel_sp)
        , kernel_lr(stop_addr)
        , xn_reversed {0} {};

    bool is_kernel_registers() const { return user_sp == kernel_sp; };

    SPSR_EL1 cpsr;  // The SPSR containing the PSTATE return to (either user or kernel, depending on if starting a task)
    u64 pc;         // The PC to return to (either user or kernel, depending on if starting a task)
    u64 user_sp;
    u64 kernel_sp;
    u64 kernel_lr;
    u64 xn_reversed[30];
};

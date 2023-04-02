#pragma once
#include <pine/bit.hpp>
#include <pine/print.hpp>
#include <pine/types.hpp>

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

struct SPSR_EL1 {
    static SPSR_EL1 from_data(PtrData spsr)
    {
        return pine::bit_cast<SPSR_EL1>(spsr);
    }

    friend void print_with(pine::Printer& printer, const SPSR_EL1& cpsr);

    u64 m : 4;              // 0-3: AArch64 Exception level and selected Stack Pointer.
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
};

struct Registers {
    u64 xn[30];
    u64 lr;
    u64 zero;

    friend void print_with(pine::Printer& printer, const Registers& registers);
};

static_assert(sizeof(SPSR_EL1) == 8);
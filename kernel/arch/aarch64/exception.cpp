#include "exception.hpp"
#include "kernel/device/interrupts.hpp"
#include "kernel/interrupt_disabler.hpp"
#include "panic.hpp"

extern "C" {

void data_abort_handler(const Registers& registers)
{
    // FIXME: Print out MMU table here
    panic("\033[31mData abort! halting.\033[0m\n\n", registers);
}

void undefined_instruction_handler(const Registers& registers)
{
    panic("\033[31mUndefined instruction! halting.\033[0m\n\n", registers);
}

void synchronous_kernel_handler(const Registers& registers)
{
    ESR_EL1 esr {};
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    switch (esr.ec) {
    case ExceptionClass::UnknownReason:
        panic("\033[31mException raised with unknown reason! halting.\033[0m\n\n", registers);
        break;
    case ExceptionClass::SIMDException:
    case ExceptionClass::FPException:
        panic("\033[31mFP or SIMD code attempted to execute! halting.\033[0m\n\n", registers);
        break;
    case ExceptionClass::SVC: {
        static int already_called = false;
        if (!already_called) {
            already_called = true;
            asm volatile("svc 1");
        }
        return;
    }
    case ExceptionClass::InstructionAbortEL0:
    case ExceptionClass::InstructionAbort:
        undefined_instruction_handler(registers);
        break;
    case ExceptionClass::DataAbortEL0:
    case ExceptionClass::DataAbort:
        data_abort_handler(registers);
        break;
    case ExceptionClass::SError:
        panic("\033[31mException raised. halting.\033[0m\n\n", registers);
        break;
    default:
        panic("\033[31mUnknown exception raised! (", (PtrData) esr.ec, ") halting.\033[0m\n\n", registers);
    }
}

void irq_handler()
{
    interrupts_handle_irq(InterruptsDisabledTag::promise());
}

}

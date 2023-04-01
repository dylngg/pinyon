#include "exception.hpp"
#include "panic.hpp"

extern "C" {

void data_abort_handler()
{
    // FIXME: Print out MMU table here
    panic("\033[31mData abort! halting.\033[0m\n\n");
}

void undefined_instruction_handler()
{
    panic("\033[31mUndefined instruction! halting.\033[0m\n\n");
}


void kernel_synchronous_handler(ESR_EL1 esr)
{
    switch (esr.ec) {
    case ExceptionClass::UnknownReason:
        panic("\033[31mException raised with unknown reason! halting.\033[0m\n\n");
        break;
    case ExceptionClass::SIMDException:
    case ExceptionClass::FPException:
        panic("\033[31mFP or SIMD code attempted to execute! halting.\033[0m\n\n");
        break;
    case ExceptionClass::SVC:
        return;
    case ExceptionClass::InstructionAbortEL0:
    case ExceptionClass::InstructionAbort:
        undefined_instruction_handler();
        break;
    case ExceptionClass::DataAbortEL0:
    case ExceptionClass::DataAbort:
        data_abort_handler();
        break;
    case ExceptionClass::SError:
        panic("\033[31mException raised. halting.\033[0m\n\n");
        break;
    default:
        panic("\033[31mUnknown exception raised! halting.\033[0m\n\n");
    }
}

}

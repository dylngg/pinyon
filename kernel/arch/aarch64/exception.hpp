#pragma once
#include "processor.hpp"

extern "C" {
void synchronous_userspace_handler(Syscall call, PtrData arg1, PtrData arg2, PtrData arg3, ExceptionSavedRegisters&);
void synchronous_kernel_handler(const ExceptionSavedRegisters&);

void irq_handler();
}
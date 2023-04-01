#pragma once

#include "processor.hpp"
#include "../../console.hpp"

#include <pine/print.hpp>
#include <pine/utility.hpp>

template <typename... Args>
inline void panic(Args&& ... args)
{
    PtrData sp;
    PtrData lr;  // Collect before clobbered by calls
    PtrData cpsr;
    asm volatile("mov %0, sp"
                 : "=r"(sp));
    asm volatile("mov %0, lr"
                 : "=r"(lr));
    asm volatile("mrs %0, cpsr"
                 : "=r"(cpsr));

    consoleln("\nKERNEL PANIC! (!!!)");
    consoleln(pine::forward<Args>(args)...);

    consoleln("cpsr:", CPSR::from_data(cpsr), "sp:", reinterpret_cast<void*>(sp), "lr:", reinterpret_cast<void*>(lr));
    asm volatile("b halt");
}

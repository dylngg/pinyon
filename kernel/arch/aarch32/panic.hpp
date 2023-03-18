#pragma once

#include "../../processor.hpp"
#include "../../console.hpp"

#include <pine/print.hpp>
#include <pine/utility.hpp>

template <typename... Args>
inline void panic(Args&& ... args)
{
    auto uart_printer = UARTPrinter();
    PtrData sp;
    PtrData lr;
    PtrData cpsr;
    asm volatile("mov %0, sp"
                 : "=r"(sp));
    asm volatile("mov %0, lr"
                 : "=r"(lr));
    asm volatile("mrs %0, cpsr"
                 : "=r"(cpsr));

    print_with(uart_printer, "\nKERNEL PANIC! (!!!)\n\n");
    print_each_with(uart_printer, pine::forward<Args>(args)...);

    consoleln("\n\ncpsr:", CPSR::from_data(cpsr), "sp:", reinterpret_cast<void*>(sp), "lr:", reinterpret_cast<void*>(lr));
    asm volatile("b halt");
}

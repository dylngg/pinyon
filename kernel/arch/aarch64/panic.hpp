#pragma once

#include "processor.hpp"
#include "../../console.hpp"

#include <pine/print.hpp>
#include <pine/utility.hpp>

template <typename... Args>
inline void panic(Args&& ... args)
{
    PtrData daif;
    PtrData sp;
    PtrData lr;  // Collect before clobbered by calls
    PtrData el;
    PtrData spsr;
    asm volatile("mrs %0, daif" : "=r"(daif));
    asm volatile("mov %0, sp" : "=r"(sp));
    asm volatile("mov %0, lr" : "=r"(lr));
    asm volatile("mrs %0, currentel" : "=r"(el));
    asm volatile("mrs %0, spsr_el1" : "=r"(spsr));

    consoleln("\nKERNEL PANIC! (!!!)");
    consoleln(pine::forward<Args>(args)...);

    console_join("el: ", el >> 2, " daif: 0b", daif & 1 ? "1" : "0", daif & (1 << 1) ? "1" : "0", daif & (1 << 2) ? "1" : "0", daif & (1 << 3) ? "1" : "0"
                 " sp: ", reinterpret_cast<void*>(sp), " lr: ", reinterpret_cast<void*>(lr), "\n");
    console_join("spsr: ", SPSR_EL1::from_data(spsr), " ");

    asm volatile("b halt");
}


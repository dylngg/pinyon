#include "processor.hpp"

void print_with(pine::Printer& printer, const SPSR_EL1& spsr)
{
    print_with(printer, "SPSR(mode: ");
    print_with(printer, spsr.m);
    constexpr unsigned bufsize = 24;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, ", fiad:%d%d%d%d, vczn:%d%d%d%d)",
                     spsr.f, spsr.i, spsr.a, spsr.d,
                     spsr.v, spsr.c, spsr.z, spsr.n);
    print_with(printer, buf);
}

void print_with(pine::Printer& printer, const ProcessorMode& mode)
{
    const char* mode_str = "";
    switch(mode) {
    case ProcessorMode::EL0:
        mode_str = "EL0";
        break;
    case ProcessorMode::EL1h:
        mode_str = "EL1h";
        break;
    case ProcessorMode::EL1t:
        mode_str = "EL1t";
        break;
    }
    print_with(printer, mode_str);
}

void print_with(pine::Printer& printer, const ExceptionSavedRegisters& registers)
{
    // Cast to void* to enable hexadecimal printing
    print_each_with_spacing(printer, "x0: ", (void*) registers.xn[0], "x1: ", (void*) registers.xn[1], "x2: ", (void*) registers.xn[2], "x3: ", (void*) registers.xn[3]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x4: ", (void*) registers.xn[4], "x5: ", (void*) registers.xn[5], "x6: ", (void*) registers.xn[6], "x7: ", (void*) registers.xn[7]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x8: ", (void*) registers.xn[8], "x9: ", (void*) registers.xn[9], "x10:", (void*) registers.xn[10], "x11:", (void*) registers.xn[11]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x12:", (void*) registers.xn[12], "x13:", (void*) registers.xn[13], "x14:", (void*) registers.xn[14], "x15:", (void*) registers.xn[15]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x16:", (void*) registers.xn[16], "x17:", (void*) registers.xn[17], "x18:", (void*) registers.xn[18], "x19:", (void*) registers.xn[19]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x20:", (void*) registers.xn[20], "x21:", (void*) registers.xn[21], "x22:", (void*) registers.xn[22], "x23:", (void*) registers.xn[23]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x24:", (void*) registers.xn[24], "x25:", (void*) registers.xn[25], "x26:", (void*) registers.xn[26], "x27:", (void*) registers.xn[27]);
    print_with(printer, "\n");
    print_each_with_spacing(printer, "x28:", (void*) registers.xn[28], "fp: ", (void*) registers.xn[29], "lr: ", (void*) registers.lr);
    print_with(printer, "\n\n");
}

PtrData InterruptDisabler::status()
{
    PtrData state;
    asm volatile("mrs %0, daif" : "=r"(state));
    return state;
}

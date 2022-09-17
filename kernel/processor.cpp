#include "processor.hpp"


void print_with(pine::Printer& printer, const CPSR& cpsr)
{
    print_with(printer, "CPSR(mode: ");
    print_with(printer, cpsr.m);
    constexpr unsigned bufsize = 24;
    char buf[bufsize];
    pine::sbufprintf(buf, bufsize, ", fia:%d%d%d, qvczn:%d%d%d%d%d)",
                     cpsr.t, cpsr.i, cpsr.a,
                     cpsr.q, cpsr.v, cpsr.c, cpsr.z, cpsr.n);
    print_with(printer, buf);
}

void print_with(pine::Printer& printer, const ProcessorMode& mode)
{
    const char* mode_str = "";
    switch(mode) {
    case ProcessorMode::User:
        mode_str = "user";
        break;
    case ProcessorMode::FIQ:
        mode_str = "fiq";
        break;
    case ProcessorMode::IRQ:
        mode_str = "irq";
        break;
    case ProcessorMode::Supervisor:
        mode_str = "super";
        break;
    case ProcessorMode::Monitor:
        mode_str = "monitor";
        break;
    case ProcessorMode::Abort:
        mode_str = "abort";
        break;
    case ProcessorMode::Hypervisor:
        mode_str = "hyper";
        break;
    case ProcessorMode::Undefined:
        mode_str = "undef";
        break;
    case ProcessorMode::System:
        mode_str = "sys";
        break;
    }
    print_with(printer, mode_str);
}



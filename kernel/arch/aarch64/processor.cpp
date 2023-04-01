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

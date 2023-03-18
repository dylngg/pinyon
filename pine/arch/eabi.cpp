#include "c_builtins.hpp"

extern "C" {

void __aeabi_memclr4(void* dest, size_t size)
{
    bzero(dest, size);
}

void __aeabi_memclr8(void* dest, size_t size)
{
    bzero(dest, size);
}

void __aeabi_memcpy4(void* to, const void* __restrict__ from, size_t size)
{
    memcpy(to, from, size);
}

}

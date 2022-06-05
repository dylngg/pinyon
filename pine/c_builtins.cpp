#include "c_builtins.hpp"

void bzero(void* target, size_t size)
{
    unsigned char* _target = (unsigned char*)target;
    while (size-- > 0)
        *_target = 0;
}

void* memcpy(void* __restrict__ to, const void* __restrict__ from, size_t size)
{
    unsigned char* _to = (unsigned char*)to;
    unsigned char* _from = (unsigned char*)from;
    while (size-- > 0) {
        *_to = *_from;
        _to++;
        _from++;
    }

    return to;
}

void* memset(void* to, int c, size_t size)
{
    unsigned char* _to = (unsigned char*)to;
    unsigned char c_ = (unsigned char)c;
    while (size-- > 0)
        *_to++ = c_;

    return to;
}

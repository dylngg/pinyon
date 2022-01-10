#pragma once
#include <cstddef>  // for size_t

/*
 * GCC (and perhaps Clang) will often try and emit these C functions in certain
 * cases, despite our -ffreestanding and -fno-stdlib environment.
 *
 * If one were not paying close attention to the docs, one might assume that
 * these functions could be replaced with __builtin_* counterparts [0] in their
 * implementation. But, despite the builtin name, these calls only expand to
 * efficent inline assembly implementations in some cases, not all, meaning we
 * have to provide reimplements here. At least that is my understanding.
 *
 * [0] https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
 */

extern "C" {

void bzero(void* target, size_t size);
void* memcpy(void* __restrict__ to, const void* __restrict__ from, size_t size);
void* memset(void *to, int c, size_t size);

}
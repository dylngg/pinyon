#pragma once

// Note: Headers such as <c...> comes from GCC's builtin functions when using
//       -ffrestanding: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
//       In clang with -ffreestanding, these are not defined, but the C ones
//       are.
#ifdef CLANG_HAS_NO_CXX_INCLUDES
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef> // size_t, ptrdiff_t, offsetof
#include <cstdint> // int types
#endif

/*
 * Define easier to type aliases for sized integers here.
 */
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

// We do necessary pointer arithmetic that we cannot do with void* unless
// we're ok with the compiler constantly screaming at us.
using PtrData = uintptr_t;
using ssize_t = intptr_t;

template <class First, class Second>
struct Pair {
    First first;
    Second second;
};

template <class First, class Second, class Third>
struct Triple {
    First first;
    Second second;
    Third third;
};

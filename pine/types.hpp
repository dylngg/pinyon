#pragma once

#include <cstddef> // size_t
#include <cstdint> // int types

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

// ARCH: 32-bit dependent
// We do necessary pointer arithmetic that we cannot do with void* unless
// we're ok with the compiler constantly screaming at us.
using PtrData = uintptr_t;

template <class First, class Second>
struct Pair {
    First first;
    Second second;
};

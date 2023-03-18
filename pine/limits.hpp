#pragma once

// Note: Headers such as <c...> comes from GCC's builtin functions when using
//       -ffrestanding: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
//       In clang with -ffreestanding, these are not defined, but the C ones
//       are.
#ifdef CLANG_HAS_NO_CXX_INCLUDES
#include <limits.h>
#else
#include <climits>
#endif

#include "math.hpp"

namespace pine {

template <typename = void>
struct limits {
};

template <>
struct limits<bool> {
    static constexpr bool min = false;
    static constexpr bool max = true;
    static constexpr unsigned width = sizeof(bool) / CHAR_BIT;
    static constexpr int digits10 = 1;  // 0 or 1
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(bool) * 2;
};

template <>
struct limits<char> {
    static constexpr char min = CHAR_MIN;
    static constexpr char max = CHAR_MAX;
    static constexpr unsigned width = sizeof(char) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    // char may or may not be signed, depending on the platform
    static constexpr int characters10 = digits10 + is_signed<char>;
    static constexpr int digits16 = sizeof(char) * 2;
};

template <>
struct limits<unsigned char> {
    static constexpr unsigned char min = 0;
    static constexpr unsigned char max = UCHAR_MAX;
    static constexpr unsigned width = sizeof(unsigned char) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(unsigned char) * 2;
};

template <>
struct limits<signed char> {
    static constexpr signed char min = SCHAR_MIN;
    static constexpr signed char max = SCHAR_MAX;
    static constexpr unsigned width = sizeof(signed char) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 2;
    static constexpr int characters10 = digits10 + 1;  // + '-'
    static constexpr int digits16 = sizeof(signed char) * 2;
};

template <>
struct limits<short> {
    static constexpr short min = SHRT_MIN;
    static constexpr short max = SHRT_MAX;
    static constexpr unsigned width = sizeof(short) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 2;
    static constexpr int characters10 = digits10 + 1;  // + '-'
    static constexpr int digits16 = sizeof(short) * 2;
};

template <>
struct limits<unsigned short> {
    static constexpr unsigned short min = 0;
    static constexpr unsigned short max = USHRT_MAX;
    static constexpr unsigned width = sizeof(unsigned short) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(unsigned short) * 2;
};

template <>
struct limits<int> {
    static constexpr int min = INT_MIN;
    static constexpr int max = INT_MAX;
    static constexpr unsigned width = sizeof(int) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10 + 1;  // + '-'
    static constexpr int digits16 = sizeof(int) * 2;
};

template <>
struct limits<unsigned int> {
    static constexpr unsigned int min = 0;
    static constexpr unsigned int max = UINT_MAX;
    static constexpr unsigned width = sizeof(unsigned int) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(unsigned int) * 2;
};

template <>
struct limits<long> {
    static constexpr long min = LONG_MIN;
    static constexpr long max = LONG_MAX;
    static constexpr unsigned width = sizeof(long) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10 + 1;  // + '-'
    static constexpr int digits16 = sizeof(long) * 2;
};

template <>
struct limits<unsigned long> {
    static constexpr unsigned long min = 0;
    static constexpr unsigned long max = ULONG_MAX;
    static constexpr unsigned width = sizeof(unsigned long) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(unsigned long) * 2;
};

template <>
struct limits<long long> {
    static constexpr long long min = LLONG_MIN;
    static constexpr long long max = LLONG_MAX;
    static constexpr unsigned width = sizeof(long long) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10 + 1;  // + '-'
    static constexpr int digits16 = sizeof(long long) * 2;
};

template <>
struct limits<unsigned long long> {
    static constexpr unsigned long long min = 0;
    static constexpr unsigned long long max = ULLONG_MAX;
    static constexpr unsigned width = sizeof(unsigned long long) / CHAR_BIT;
    static constexpr int digits10 = log(max) + 1;
    static constexpr int characters10 = digits10;
    static constexpr int digits16 = sizeof(unsigned long long) * 2;
};

}

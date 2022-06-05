#pragma once
#include "metaprogramming.hpp"

namespace pine {

template <typename Int, enable_if<is_signed<Int>, Int>* = nullptr>
constexpr Int abs(Int num)
{
    return num >= 0 ? num : -num;
}

template <typename Int, typename Int2 = Int, enable_if<is_integer<Int>, Int>* = nullptr>
constexpr Int pow(Int num, Int2 power)
{
    if (power == 0)
        return 1;

    Int result = num;
    while (power > 1) {
        result *= num;
        --power;
    }
    return result;
}

template <typename Int, Int base = 10, enable_if<is_integer<Int> && !is_same<Int, bool>, Int>* = nullptr>
constexpr Int log(Int num)
{
    // FIXME: Specialize for base two
    Int result = 0;
    while (num >= base) {
        // log_base(x) = y if b^y = x
        num /= base;
        ++result;
    }
    return result;
}

template <typename SizeType>
constexpr const SizeType& max(const SizeType& first, const SizeType& second)
{
    return (first > second) ? first : second;
}

template <typename SizeType>
constexpr const SizeType& min(const SizeType& first, const SizeType& second)
{
    return (first < second) ? first : second;
}

}

#pragma once
#include "c_builtins.hpp"
#include "metaprogramming.hpp"

namespace pine {

template <typename To, typename From>
constexpr To bit_cast(From from)
{
    static_assert(sizeof(To) == sizeof(From));
    To to;
    memcpy (&to, &from, sizeof(To));
    return to;
}

template <typename UInt, typename UInt2>
constexpr UInt overwrite_bit_range(UInt dest, UInt2 value, int start_inclusive, int end_inclusive)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2>);
    UInt mask = ((1u << (end_inclusive - start_inclusive)) - 1) << start_inclusive;
    return (dest & ~mask) | (value & mask);
}

}

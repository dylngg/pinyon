#pragma once
#include "types.hpp"

// FIXME: Use template metaprogramming to ensure the UInt types are unsigned

template <typename UInt, typename UInt2>
constexpr UInt align_down_two(UInt value, UInt2 base_two_num)
{
    return value & ~(base_two_num - 1);
}

template <typename UInt, typename UInt2>
constexpr UInt align_up_two(UInt value, UInt2 base_two_num)
{
    return (value + (base_two_num - 1)) & ~(base_two_num - 1);
}

template <typename UInt, typename UInt2>
constexpr bool is_aligned_two(UInt value, UInt2 base_two_num)
{
    return (value & (base_two_num - 1)) == 0;
}

template <typename UInt, typename UInt2>
constexpr UInt overwrite_bit_range(UInt dest, UInt2 value, int start_inclusive, int end_inclusive)
{
    UInt mask = ((1 << (end_inclusive - start_inclusive)) - 1) << start_inclusive;
    return (dest & ~mask) | (value & mask);
}
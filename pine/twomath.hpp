#pragma once
#include "types.hpp"

// FIXME: Use template metaprogramming to ensure the UInt types are unsigned

template <typename UInt>
constexpr UInt align_down_two(UInt value, UInt base_two_num)
{
    return value & ~(base_two_num - 1);
}

template <typename UInt>
constexpr UInt align_up_two(UInt value, UInt base_two_num)
{
    return (value + (base_two_num - 1)) & ~(base_two_num - 1);
}

template <typename UInt>
constexpr bool is_aligned_two(UInt value, UInt base_two_num)
{
    return (value & (base_two_num - 1)) == 0;
}

template <typename UInt>
constexpr UInt overwrite_bit_range(UInt dest, UInt value, int start, int end)
{
    UInt mask = ((1 << (end - start)) - 1) << start;
    return (dest & ~mask) | (value & mask);
}
#pragma once
#include "types.hpp"

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
#pragma once
#include "types.hpp"
#include "metaprogramming.hpp"
#include "limits.hpp"

namespace pine {

template <typename UInt, typename UInt2>
constexpr UInt align_down_two(UInt value, UInt2 base_two_num)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2> && is_convertible_to<UInt2, UInt>);
    auto alignment = static_cast<UInt>(base_two_num);
    // simply mask off all after two bit.
    // e.g. value=7, base_two_num=8:
    //      0b111 & ~(0b0001 - 1)
    //      0b111 & ~(0b111)
    //      0b111 & 0b0001
    //      = 0b0000
    if (value == 0 || alignment == 0)
        return 0;
    return value & ~(alignment - 1);
}

template <typename UInt, typename UInt2>
constexpr UInt align_up_two(UInt value, UInt2 base_two_num)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2> && is_convertible_to<UInt2, UInt>);
    auto alignment = static_cast<UInt>(base_two_num);
    // mask off two-wrapped value after two bit
    // e.g. value=7, base_two_num=8:
    //      0b111 + (0b0001 - 1) & ~(0b0001 - 1)
    //      0b111 + 0b111 & ~(0b111)
    //      0b1110 & 0b0001
    //      = 0b0001
    if (value == 0)
        return alignment;
    if (alignment == 0)
        return value;
    return (value + (alignment - 1)) & ~(alignment - 1);
}

template <typename UInt, typename UInt2>
constexpr UInt alignment_up_two(UInt value, UInt2 base_two_num)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2> && is_convertible_to<UInt2, UInt>);
    auto alignment = static_cast<UInt>(base_two_num);
    // inverse of bits before two bit; if 0 will return 0
    if (alignment == 0)
        return 0;
    return align_up_two(value, alignment) - value;
}

template <typename UInt, typename UInt2>
constexpr bool is_aligned_two(UInt value, UInt2 base_two_num)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2> && is_convertible_to<UInt2, UInt>);
    auto alignment = static_cast<UInt>(base_two_num);
    return value == 0 || alignment_up_two(value, alignment) == 0;
}

// FIXME: Create specialization of is_aligned_two
template <typename Ptr, typename UInt2>
constexpr bool is_pointer_aligned_two(Ptr ptr, UInt2 base_two_num)
{
    static_assert(is_pointer<Ptr> && is_unsigned<UInt2> && is_convertible_to<UInt2, PtrData>);
    auto alignment = static_cast<PtrData>(base_two_num);
    return is_aligned_two<PtrData, PtrData>(reinterpret_cast<PtrData>(ptr), alignment);
}

template <typename UInt>
constexpr bool is_aligned_two_power(UInt value)
{
    static_assert(is_unsigned<UInt>);
    return !(value & (value - 1));
}

template <typename UInt, typename UInt2>
constexpr UInt overwrite_bit_range(UInt dest, UInt2 value, int start_inclusive, int end_inclusive)
{
    static_assert(is_unsigned<UInt> && is_unsigned<UInt2>);
    UInt mask = ((1 << (end_inclusive - start_inclusive)) - 1) << start_inclusive;
    return (dest & ~mask) | (value & mask);
}

template <typename UInt>
constexpr UInt bit_width(UInt value)
{
    static_assert(is_unsigned<UInt>);
    unsigned width = 0;
    unsigned count = 1;
    while (value != 0) {
        if (value & 1)
            width = count;

        ++count;
        value >>= 1;
    }
    return static_cast<UInt>(width);
}

template <typename UInt>
constexpr UInt align_down_to_power(UInt value)
{
    static_assert(is_unsigned<UInt>);
    if (value == 0)
        return 0;

    return 1u << (bit_width(value) - 1u);
}

template <typename UInt>
constexpr UInt align_up_to_power(UInt value)
{
    static_assert(is_unsigned<UInt>);
    if (value == 0)
        return 1;

    auto aligned_down = align_down_to_power(value);
    return aligned_down == value ? value : aligned_down << 1;
}

}

#pragma once

#include <cassert>
#include <climits>

#include <pine/twomath.hpp>

using namespace pine;

void align_down_two()
{
    assert(align_down_two(17u, 8u) == 16);
    assert(align_down_two(18u, 8u) == 16);
    assert(align_down_two(19u, 8u) == 16);
    assert(align_down_two(20u, 8u) == 16);
    assert(align_down_two(21u, 8u) == 16);
    assert(align_down_two(22u, 8u) == 16);
    assert(align_down_two(23u, 8u) == 16);
    assert(align_down_two(24u, 8u) == 24);

    assert(align_down_two(1u, 8u) == 0);
    assert(align_down_two(0u, 8u) == 0);
    assert(align_down_two(15u, 8u) == 8);
    assert(align_down_two(16u, 8u) == 16);
    assert(align_down_two(34u, 8u) == 32);
    assert(align_down_two(33u, 8u) == 32);
    assert(align_down_two(8u, 8u) == 8);
    assert(align_down_two(33u, 16u) == 32);

    assert(align_down_two(0u, 2u) == 0);
    assert(align_down_two(1u, 2u) == 0);
    assert(align_down_two(2u, 2u) == 2);
    assert(align_down_two(3u, 2u) == 2);
    assert(align_down_two(7u, 2u) == 6);
    assert(align_down_two(8u, 2u) == 8);

    assert(align_down_two(0u, 1u) == 0);
    assert(align_down_two(1u, 1u) == 1);
    assert(align_down_two(2u, 1u) == 2);
    assert(align_down_two(3u, 1u) == 3);
    assert(align_down_two(7u, 1u) == 7);
    assert(align_down_two(8u, 1u) == 8);

    assert(align_down_two(0u, 0u) == 0);
    assert(align_down_two(1u, 0u) == 0);
    assert(align_down_two(2u, 0u) == 0);
    assert(align_down_two(8u, 0u) == 0);

    unsigned large_num = 1u << (sizeof(large_num) * CHAR_BIT - 1);
    assert(align_down_two(large_num + 5, 0u) == 0);
}

void alignment_up_two()
{
    assert(alignment_up_two(17u, 8u) == 7);
    assert(alignment_up_two(18u, 8u) == 6);
    assert(alignment_up_two(19u, 8u) == 5);
    assert(alignment_up_two(20u, 8u) == 4);
    assert(alignment_up_two(21u, 8u) == 3);
    assert(alignment_up_two(22u, 8u) == 2);
    assert(alignment_up_two(23u, 8u) == 1);
    assert(alignment_up_two(24u, 8u) == 0);

    assert(alignment_up_two(1u, 8u) == 7);
    assert(alignment_up_two(0u, 8u) == 8);
    assert(alignment_up_two(15u, 8u) == 1);
    assert(alignment_up_two(16u, 8u) == 0);
    assert(alignment_up_two(34u, 8u) == 6);
    assert(alignment_up_two(33u, 8u) == 7);
    assert(alignment_up_two(8u, 8u) == 0);
    assert(alignment_up_two(33u, 16u) == 15);

    assert(alignment_up_two(0u, 2u) == 2);
    assert(alignment_up_two(1u, 2u) == 1);
    assert(alignment_up_two(2u, 2u) == 0);
    assert(alignment_up_two(3u, 2u) == 1);
    assert(alignment_up_two(7u, 2u) == 1);
    assert(alignment_up_two(8u, 2u) == 0);

    assert(alignment_up_two(0u, 1u) == 1);
    assert(alignment_up_two(1u, 1u) == 0);
    assert(alignment_up_two(2u, 1u) == 0);
    assert(alignment_up_two(3u, 1u) == 0);
    assert(alignment_up_two(7u, 2u) == 1);
    assert(alignment_up_two(8u, 1u) == 0);

    assert(alignment_up_two(0u, 0u) == 0);
    assert(alignment_up_two(1u, 0u) == 0);
    assert(alignment_up_two(2u, 0u) == 0);
    assert(alignment_up_two(5u, 0u) == 0);
    assert(alignment_up_two(8u, 0u) == 0);

    unsigned large_num = 1u << (sizeof(large_num) * CHAR_BIT - 1);
    assert(alignment_up_two(large_num + 5, 0u) == 0);
}

void align_up_two()
{
    assert(align_up_two(17u, 8u) == 24);
    assert(align_up_two(18u, 8u) == 24);
    assert(align_up_two(19u, 8u) == 24);
    assert(align_up_two(20u, 8u) == 24);
    assert(align_up_two(21u, 8u) == 24);
    assert(align_up_two(22u, 8u) == 24);
    assert(align_up_two(23u, 8u) == 24);
    assert(align_up_two(24u, 8u) == 24);

    assert(align_up_two(1u, 8u) == 8);
    assert(align_up_two(0u, 8u) == 8);
    assert(align_up_two(15u, 8u) == 16);
    assert(align_up_two(16u, 8u) == 16);
    assert(align_up_two(34u, 8u) == 40);
    assert(align_up_two(33u, 8u) == 40);
    assert(align_up_two(8u, 8u) == 8);
    assert(align_up_two(33u, 16u) == 48);

    assert(align_up_two(0u, 2u) == 2);
    assert(align_up_two(1u, 2u) == 2);
    assert(align_up_two(2u, 2u) == 2);
    assert(align_up_two(3u, 2u) == 4);
    assert(align_up_two(7u, 2u) == 8);
    assert(align_up_two(8u, 2u) == 8);

    assert(align_up_two(0u, 1u) == 1);
    assert(align_up_two(1u, 1u) == 1);
    assert(align_up_two(2u, 1u) == 2);
    assert(align_up_two(3u, 1u) == 3);
    assert(align_up_two(7u, 1u) == 7);
    assert(align_up_two(8u, 1u) == 8);

    assert(align_up_two(0u, 0u) == 0);
    assert(align_up_two(1u, 0u) == 1);
    assert(align_up_two(2u, 0u) == 2);
    assert(align_up_two(8u, 0u) == 8);

    unsigned large_num = 1u << (sizeof(large_num) * CHAR_BIT - 1);
    assert(align_up_two(large_num + 5, 0u) == large_num + 5);
}

void is_aligned_two()
{
    assert(!is_aligned_two(17u, 8u));
    assert(!is_aligned_two(18u, 8u));
    assert(!is_aligned_two(19u, 8u));
    assert(!is_aligned_two(20u, 8u));
    assert(!is_aligned_two(21u, 8u));
    assert(!is_aligned_two(22u, 8u));
    assert(!is_aligned_two(23u, 8u));
    assert(is_aligned_two(24u, 8u));

    assert(!is_aligned_two(1u, 8u));
    assert(is_aligned_two(0u, 8u));
    assert(!is_aligned_two(15u, 8u));
    assert(is_aligned_two(16u, 8u));
    assert(!is_aligned_two(34u, 8u));
    assert(!is_aligned_two(33u, 8u));
    assert(is_aligned_two(8u, 8u));
    assert(!is_aligned_two(33u, 16u));

    assert(is_aligned_two(0u, 2u));
    assert(!is_aligned_two(1u, 2u));
    assert(is_aligned_two(2u, 2u));
    assert(!is_aligned_two(3u, 2u));
    assert(!is_aligned_two(7u, 2u));
    assert(is_aligned_two(8u, 2u));

    assert(is_aligned_two(0u, 1u));
    assert(is_aligned_two(1u, 1u));
    assert(is_aligned_two(2u, 1u));
    assert(is_aligned_two(3u, 1u));
    assert(is_aligned_two(7u, 1u));
    assert(is_aligned_two(8u, 1u));

    assert(is_aligned_two(0u, 0u));
    assert(is_aligned_two(1u, 0u));
    assert(is_aligned_two(2u, 0u));
    assert(is_aligned_two(8u, 0u));

    unsigned large_num = 1u << (sizeof(large_num) * CHAR_BIT - 1);
    assert(is_aligned_two(large_num + 5, 0u));
}

void bit_width()
{
    assert(bit_width(0u) == 0);
    assert(bit_width(1u) == 1);
    assert(bit_width(2u) == 2);
    assert(bit_width(3u) == 2);
    assert(bit_width(4u) == 3);
    assert(bit_width(5u) == 3);
    assert(bit_width(6u) == 3);
    assert(bit_width(7u) == 3);
    assert(bit_width(8u) == 4);
    assert(bit_width(16u) == 5);
    assert(bit_width(25u) == 5);
    assert(bit_width(31u) == 5);
    assert(bit_width(32u) == 6);
    assert(bit_width(33u) == 6);
    assert(bit_width(ULONG_MAX) == sizeof(unsigned long) * CHAR_BIT);
}

void align_down_to_power()
{
    assert(align_down_to_power(17u) == 16);
    assert(align_down_to_power(18u) == 16);
    assert(align_down_to_power(19u) == 16);
    assert(align_down_to_power(20u) == 16);
    assert(align_down_to_power(21u) == 16);
    assert(align_down_to_power(22u) == 16);
    assert(align_down_to_power(23u) == 16);
    assert(align_down_to_power(24u) == 16);

    assert(align_down_to_power(1u) == 1);
    assert(align_down_to_power(0u) == 0);
    assert(align_down_to_power(15u) == 8);
    assert(align_down_to_power(16u) == 16);
    assert(align_down_to_power(34u) == 32);
    assert(align_down_to_power(33u) == 32);
    assert(align_down_to_power(8u) == 8);
    assert(align_down_to_power(33u) == 32);

    assert(align_down_to_power(0u) == 0);
    assert(align_down_to_power(1u) == 1);
    assert(align_down_to_power(2u) == 2);
    assert(align_down_to_power(3u) == 2);
    assert(align_down_to_power(7u) == 4);
    assert(align_down_to_power(8u) == 8);

    unsigned large_num = 1u << (sizeof(large_num) * CHAR_BIT - 1);
    assert(align_down_to_power(large_num + 5) == large_num);
}

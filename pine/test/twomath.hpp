#pragma once

#include <cassert>
#include <climits>

#include <pine/twomath.hpp>

using namespace pine;

void align_down_two()
{
    assert(align_down_two(17u, 8u) == 16);
    assert(align_down_two(16u, 8u) == 16);
    assert(align_down_two(15u, 8u) == 8);
    assert(align_down_two(33u, 8u) == 32);
    assert(align_down_two(0u, 8u) == 0);
    assert(align_down_two(1u, 8u) == 0);
    assert(align_down_two(8u, 8u) == 8);
}

void align_up_two()
{
    assert(align_up_two(17u, 8u) == 24);
    assert(align_up_two(16u, 8u) == 16);
    assert(align_up_two(15u, 8u) == 16);
    assert(align_up_two(33u, 8u) == 40);
    assert(align_up_two(0u, 8u) == 0);
    assert(align_up_two(1u, 8u) == 8);
    assert(align_up_two(8u, 8u) == 8);
}

void is_aligned_two()
{
    assert(!is_aligned_two(17u, 8u));
    assert(is_aligned_two(16u, 8u));
    assert(!is_aligned_two(15u, 8u));
    assert(!is_aligned_two(33u, 8u));
    assert(is_aligned_two(0u, 8u));
    assert(!is_aligned_two(1u, 8u));
    assert(is_aligned_two(8u, 8u));
}

void bit_width()
{
    assert(bit_width(0u) == 0);
    assert(bit_width(1u) == 1);
    assert(bit_width(2u) == 2);
    assert(bit_width(3u) == 2);
    assert(bit_width(7u) == 3);
    assert(bit_width(8u) == 4);
    assert(bit_width(ULONG_MAX) == sizeof(unsigned long)*CHAR_BIT);
}

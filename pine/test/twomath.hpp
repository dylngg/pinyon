#include <cassert>
#include <pine/twomath.hpp>

void align_down_two()
{
    assert(align_down_two(17, 8) == 16);
    assert(align_down_two(16, 8) == 16);
    assert(align_down_two(15, 8) == 8);
    assert(align_down_two(33, 8) == 32);
    assert(align_down_two(0, 8) == 0);
    assert(align_down_two(1, 8) == 0);
    assert(align_down_two(8, 8) == 8);
}

void align_up_two()
{
    assert(align_up_two(17, 8) == 24);
    assert(align_up_two(16, 8) == 16);
    assert(align_up_two(15, 8) == 16);
    assert(align_up_two(33, 8) == 40);
    assert(align_up_two(0, 8) == 0);
    assert(align_up_two(1, 8) == 8);
    assert(align_up_two(8, 8) == 8);
}

void is_aligned_two()
{
    assert(!is_aligned_two(17, 8));
    assert(is_aligned_two(16, 8));
    assert(!is_aligned_two(15, 8));
    assert(!is_aligned_two(33, 8));
    assert(is_aligned_two(0, 8));
    assert(!is_aligned_two(1, 8));
    assert(is_aligned_two(8, 8));
}
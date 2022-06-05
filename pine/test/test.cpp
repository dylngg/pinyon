#include <cstdio>

#include "twomath.hpp"
#include "maybe.hpp"

/*
 * Our poor mans test suite. Because trying to get boost test and google test
 * working was too difficult.
 *
 * TODO: Replace this test suite with something beefier and more convenient
 */

int main()
{
    align_down_two();
    align_up_two();
    is_aligned_two();
    alignment_up_two();
    bit_width();
    align_down_to_power();

    maybe_basic();
    maybe_copy_assignment();
    maybe_copy_constructor();
    maybe_move_assignment();
    maybe_move_constructor();
    printf("Success!\n");
}

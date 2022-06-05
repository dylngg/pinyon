#include "twomath.hpp"
#include "linked_list.hpp"
#include "maybe.hpp"

#include <pine/alien/print.hpp>  // Need access to our print() ADL implementations (analogus to std::cout)

/*
 * Our poor mans test suite. Because trying to get boost test and google test
 * working was too difficult.
 *
 * TODO: Replace this test suite with something beefier and more convenient
 */

int main()
{
    alien::errorln("Testing LinkedList");
    linked_list_create();
    linked_list_iterate();
    linked_list_append();
    linked_list_remove();

    alien::errorln("Testing alignment");
    align_down_two();
    align_up_two();
    is_aligned_two();
    alignment_up_two();
    bit_width();
    align_down_to_power();

    alien::errorln("Testing Maybe<>");
    maybe_basic();
    maybe_copy_assignment();
    maybe_copy_constructor();
    maybe_move_assignment();
    maybe_move_constructor();

    alien::errorln("Success!");
}

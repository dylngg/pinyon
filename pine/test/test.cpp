#include "algorithm.hpp"
#include "forward_container.hpp"
#include "linked_list.hpp"
#include "malloc.hpp"
#include "maybe.hpp"
#include "twomath.hpp"
#include "vector.hpp"

#include <pine/alien/print.hpp>  // Need access to our print() ADL implementations (analogus to std::cout)

/*
 * Our poor mans test suite. Because trying to get boost test and google test
 * working was too difficult.
 *
 * TODO: Replace this test suite with something beefier and more convenient
 */

int main()
{
    alien::errorln("Testing Vector");
    forward_container_tests<Vector<int, alien::Allocator>>();

    alien::errorln("Testing ManualLinkedList");
    // ManualLinkedList, though a forward iterator, cannot use the generic
    // forward_container_tests since it is manually allocated.
    manual_linked_list_create();
    manual_linked_list_iterate();
    manual_linked_list_append();
    manual_linked_list_remove();

    alien::errorln("Testing alignment");
    align_down_two();
    align_up_two();
    is_aligned_two();
    alignment_up_two();
    bit_width();
    align_down_to_power();
    align_up_to_power();

    alien::errorln("Testing Maybe<>");
    maybe_basic();
    maybe_copy_assignment();
    maybe_copy_constructor();
    maybe_move_assignment();
    maybe_move_constructor();

    alien::errorln("Testing FreeList");
    free_list_re_add();

    alien::errorln("Testing Algorithms");
    algorithm_find();
    algorithm_find_if();

    alien::errorln("Success!");
}

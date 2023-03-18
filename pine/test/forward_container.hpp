#pragma once

#include <pine/alien/malloc.hpp>

#include <pine/iter.hpp>

#include <cassert>
#include <new>

template <typename Container>
void forward_container_empty()
{
    Container c(alien::allocator());
    assert(c.length() == 0);
    assert(c.empty());

    assert(c.begin() == c.end());
}

template <typename Container>
void forward_container_ctor()
{
    typename Container::Iter iters[3];
    int elements[3];

    Container c1 (alien::allocator());
    assert(c1.append(1));
    assert(c1.append(2));
    assert(c1.append(3));
    assert(c1.length() == 3);
    assert(c1.begin() != c1.end());

    for (size_t i = 0; i < c1.length(); i++) {
        iters[i] = pine::next(c1.begin(), i);
        elements[i] = *iters[i];
        if (i != 0) {
            assert(elements[i] > elements[i - 1]);
            assert(pine::prev(iters[i]) == iters[i - 1]);
        }
    }
}

template <typename Container>
void forward_container_move_ctor()
{
    typename Container::Iter iters[3];
    int elements[3];

    Container c1 (alien::allocator());
    assert(c1.append(1));
    assert(c1.append(2));
    assert(c1.append(3));
    assert(c1.length() == 3);
    assert(c1.begin() != c1.end());

    for (size_t i = 0; i < c1.length(); i++) {
        iters[i] = pine::next(c1.begin(), i);
        elements[i] = *iters[i];
    }

    auto old_c1_begin = c1.begin();
    Container c3(pine::move(c1));  // move ctor
    assert(c1.length() == 0);
    assert(c1.begin() == old_c1_begin);  // stable iterator
    assert(c1.begin() == c1.end());

    assert(c3.length() == 3);
    assert(c3.begin() == old_c1_begin);
    assert(c3.begin() != c3.end());

    for (size_t i = 0; i < 3; i++) {
        auto c3_iter = pine::next(c3.begin(), i);
        assert(c3_iter == iters[i]);
        assert(*c3_iter == elements[i]);
    }
}

template <typename Container>
void forward_container_tests()
{
    static_assert(pine::is_forward_iter<typename Container::Iter>);
    forward_container_empty<Container>();
    forward_container_ctor<Container>();
    forward_container_move_ctor<Container>();
}

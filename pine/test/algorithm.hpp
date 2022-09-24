#pragma once

#include <pine/alien/malloc.hpp>

#include <pine/algorithm.hpp>
#include <pine/iter.hpp>
#include <pine/vector.hpp>

#include <cassert>

void algorithm_find()
{
    pine::Vector<int, alien::Allocator> values ( alien::allocator(), { 5, 4, 3, 2, 1, 2, 3, 4, 5 });
    auto begin = values.begin();
    auto end = values.end();

    static_assert(pine::is_random_access_iter<decltype(values)::Iter>);

    auto it = pine::find(begin, end, 5);
    assert(it != end);
    assert(it == begin);

    it = pine::find(begin, end, 0);
    assert(it == end);

    it = pine::find(begin, end, 1);
    assert(it != end);
    assert(it == next(begin, 4));

    it = pine::find(begin, end, 4);
    assert(it != end);
    assert(it == next(begin, 1));

    it = pine::find(begin, end, 3);
    assert(it != end);
    assert(it == next(begin, 2));

    it = pine::find(begin, end, 2);
    assert(it != end);
    assert(it == next(begin, 3));

    it = pine::find(begin, end, 6);
    assert(it == end);
}

void algorithm_find_if()
{
    pine::Vector<int, alien::Allocator> values( alien::allocator(), { 1, 3, 2, 4, 3, 1 } );
    auto begin = values.begin();
    auto end = values.end();

    static_assert(pine::is_random_access_iter<decltype(values)::Iter>);
    auto is_even = [](auto first) {
        return first % 2 == 0;
    };
    auto is_odd = [](auto first) {
        return first % 2 != 0;
    };
    auto is_div_by_three = [](auto first) {
        return first % 3 == 0;
    };

    auto it = pine::find_if(begin, end, is_even);
    assert(it != end);
    assert(it == next(begin, 2));

    it = pine::find_if(begin, end, is_odd);
    assert(it == begin);

    it = pine::find_if(begin, end, is_div_by_three);
    assert(it != end);
    assert(it == next(begin, 1));
}

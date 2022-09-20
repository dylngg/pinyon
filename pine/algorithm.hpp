#pragma once

#include "types.hpp"
#include "iter.hpp"

namespace pine {

template <typename Value>
struct Less {
    template <typename Second>
    bool operator()(const Value& first, const Second& second)
    {
        return first < second;
    }
};

template <typename Value>
struct EqualTo {
    template <typename Second>
    bool operator()(const Value& first, const Second& second)
    {
        return first == second;
    }
};

template <typename Iter, typename Value, typename Compare,
          enable_if<is_forward_iter<Iter>, bool> = true>
Iter lower_bound(Iter begin, Iter end, const Value& value, Compare compare)
{
    // See https://en.cppreference.com/w/cpp/algorithm/lower_bound
    auto len = distance(begin, end);
    while (len > 0) {
        auto it = begin;
        auto offset = len / 2;
        advance(it, offset);
        if (compare(*it, value)) {
            // search the right half
            begin = next(it);
            len -= offset + 1;
        }
        else {
            // search the left half
            len = offset;
        }
    }
    return begin;
}

template <typename Iter, typename Value, typename Comp = Less<Value>,
          enable_if<is_forward_iter<Iter>, bool> = true>
Iter lower_bound(Iter begin, Iter end, const Value& value)
{
    return lower_bound(begin, end, value, Comp{});
}

template <typename Iter, typename UnaryPredicate,
          enable_if<is_forward_iter<Iter>, bool> = true>
Iter find_if(Iter begin, Iter end, UnaryPredicate pred)
{
    while (begin != end) {
        if (pred(*begin))
            return begin;

        begin = next(begin);
    }
    return end;
}

template <typename Iter, typename Value>
Iter find(Iter begin, Iter end, const Value& value)
{
    return find_if(begin, end, [&](const Value& other) {
        return EqualTo<Value>{}(other, value);
    });
}

template <typename Iter>
void move(Iter old_begin, Iter old_end, Iter new_begin)
{
    while (old_begin != old_end) {
        *old_begin = pine::move(*new_begin);
        old_begin = next(old_begin);
        new_begin = next(new_begin);
    }
}

template <typename Iter>
void copy(Iter old_begin, Iter old_end, Iter new_begin)
{
    while (old_begin != old_end) {
        *old_begin = *new_begin;
        old_begin = next(old_begin);
        new_begin = next(new_begin);
    }
}

}

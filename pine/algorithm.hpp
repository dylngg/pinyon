#pragma once
#include "iter.hpp"

namespace pine {

template <typename Value>
struct EqualTo {
    bool operator()(const Value& first, const Value& second)
    {
        return first == second;
    }
};

template <typename Iter, typename UnaryPredicate,
          enable_if<is_forward_iter<Iter>, bool> = true>
Iter find_if(Iter begin, Iter end, UnaryPredicate pred)
{
    while (begin != end) {
        if (pred(*begin))
            break;

        ++begin;
    }
    return begin;
}

template <typename Iter, typename Value>
Iter find(Iter begin, Iter end, const Value& value)
{
    return find_if(begin, end, [&](const Value& other) {
        return EqualTo<Value>{}(other, value);
    });
}

}

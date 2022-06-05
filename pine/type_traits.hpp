#pragma once

#include "metaprogramming.hpp"

namespace pine {

struct Copyable {};
struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

template <class Value>
using ConditionallyCopyable = if_else<is_copy_constructible<Value>, Copyable, NonCopyable>;

struct Movable {};
struct NonMovable
{
    NonMovable() = default;
    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

template <class Value>
using ConditionallyMovable = if_else<is_move_constructible<Value>, Movable, NonMovable>;

}

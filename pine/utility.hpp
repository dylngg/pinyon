#pragma once

// This is duplicated from metaprogramming.hpp since we don't want to pull that
// monstrosity header in here and don't need this header there.
#include "metaprogramming.hpp"

/*
 * Our own std::move
 *
 * Contrary to reasonable belief, move() does not necessarily move data, but
 * rather enables L-Values to be converted to R-Values, enabling a R-Value&&
 * move constructor/assignment operator for the receiving type to be selected
 * (usually rather than the Value& copy constructor/assignment operator one),
 * allowing for smarter copying.
 *
 * See the following for details on L/R Values:
 * https://docs.microsoft.com/en-us/cpp/cpp/lvalues-and-rvalues-visual-cpp?view=msvc-170
 * https://en.cppreference.com/w/cpp/language/value_category
 *
 * And for move:
 * https://pagefault.blog/2018/03/01/common-misconception-with-cpp-move-semantics/
 */
template <typename Value>
constexpr remove_ref<Value>&& move(Value&& value)
{
    return static_cast<remove_ref<Value>&&>(value);
}

template <typename Value>
constexpr Value&& forward(remove_ref<Value>& value) // l-value
{
    return static_cast<Value&&>(value);
}
template <typename Value>
constexpr auto forward(remove_ref<Value>&& value) // r-value
{
    static_assert(!is_rvalue<Value>);
    return static_cast<Value&&>(value);
}

template <class Value, class ReplValue = Value>
constexpr Value exchange(Value& value, ReplValue&& replacement)
{
    auto tmp = move(value);
    value = forward<ReplValue>(replacement);
    return tmp;
}

template <typename Value>
constexpr void swap(Value& first, Value& second)
{
    Value temp = move(first);
    first = move(second);
    second = move(temp);
}
#pragma once

template <bool cond>
struct true_or_false_func {
    constexpr static bool value = cond;
    constexpr operator bool() { return value; }
    constexpr bool operator()() const { return value; } // e.g. truthy()
};
using truthy = true_or_false_func<true>;
using falsey = true_or_false_func<false>;

template <class Value>
struct remove_ref_func {
    using type = Value;
};
template <class Value>
struct remove_ref_func<Value&> {
    using type = Value;
};
template <class Value>
struct remove_ref_func<Value&&> {
    using type = Value;
};
template <class Value>
using remove_ref = typename remove_ref_func<Value>::type;

template <class FirstValue, class SecondValue>
struct is_same_func : falsey {
};
template <class FirstValue>
struct is_same_func<FirstValue, FirstValue> : truthy {
};
template <class FirstValue, class SecondValue>
constexpr bool is_same = is_same_func<FirstValue, SecondValue>();

template <class Value>
constexpr bool is_rvalue = is_same<Value, Value&&>;

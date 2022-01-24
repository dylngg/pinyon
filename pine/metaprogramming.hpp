#pragma once

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
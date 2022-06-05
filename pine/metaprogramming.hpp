#pragma once

#include <cstddef> // for size_t

namespace pine {

/*
 * Defines a bunch of C++17 metaprogramming functions and boolean checks.
 * Follows the STL names somewhat. Makes heavy use of 'constexpr bool'
 * expressions for conditional checks, rather than defining template
 * metafunctions with *_v variants.
 *
 * Walter E. Brown at CppCon14 is the guide to understanding most of this,
 * since these were implemented after watching that talk:
 * https://www.youtube.com/watch?v=Am2is2QCvxY
 */

// <class...> allows pretty much everything through, but the magic with this
// line is that it provides a uniform way to SFINAE specialize on a void
// defaulted parameter, enabling an easy thruthy type to be selected when
// void_t swallows something, and a falsey type when the expression within the
// void_t fails.
//
// Via Walter E. Brown; also:
// https://en.cppreference.com/w/cpp/types/void_t
template <class...>
using void_t = void;

// Stolen from a LLVM bug thread:
// https://bugs.llvm.org/show_bug.cgi?id=27798
// It appears this is also implemented in GCC:
// https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/type_traits
//
// Of note is the fact that declval returns void instead of void&& in the STL,
// hence the additional specialization; this might not be necessary for our
// purposes but it doesn't hurt
namespace impl {

template <typename Value, typename RValue = Value&&>
RValue declval_impl(int);
template <typename Value>
Value declval_impl(long);

}

template <typename Value>
auto declval() -> decltype(impl::declval_impl<Value>(0));

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

template <class Value>
struct remove_const_func {
    using type = Value;
};
template <class Value>
struct remove_const_func<const Value> {
    using type = Value;
};
template <class Value>
using remove_const = typename remove_const_func<Value>::type;

template <class Value>
struct remove_volatile_func {
    using type = Value;
};
template <class Value>
struct remove_volatile_func<volatile Value> {
    using type = Value;
};
template <class Value>
using remove_volatile = typename remove_volatile_func<Value>::type;

template <class Value>
using remove_cv = remove_const<remove_volatile<Value>>;

template <class Value>
struct remove_ptr_func {
    using type = Value;
};
template <class Value>
struct remove_ptr_func<Value*> {
    using type = Value;
};
template <class Value>
using remove_ptr = typename remove_ptr_func<remove_cv<Value>>::type;

template <class Value>  // e.g. floating point types
struct make_unsigned_func {
    using type = void;
};
template <>
struct make_unsigned_func<char> {
    using type = unsigned char;
};
template <>
struct make_unsigned_func<unsigned char> {
    using type = unsigned char;
};
template <>
struct make_unsigned_func<signed char> {
    using type = unsigned char;
};
template <>
struct make_unsigned_func<short> {
    using type = unsigned short;
};
template <>
struct make_unsigned_func<unsigned short> {
    using type = unsigned short;
};
template <>
struct make_unsigned_func<int> {
    using type = unsigned int;
};
template <>
struct make_unsigned_func<unsigned int> {
    using type = unsigned int;
};
template <>
struct make_unsigned_func<long> {
    using type = unsigned long;
};
template <>
struct make_unsigned_func<unsigned long> {
    using type = unsigned long;
};
template <>
struct make_unsigned_func<long long> {
    using type = unsigned long long;
};
template <>
struct make_unsigned_func<unsigned long long> {
    using type = unsigned long long;
};
template <>
struct make_unsigned_func<bool> {
    using type = bool;
};

template <class Value>
using make_unsigned = typename make_unsigned_func<Value>::type;

template <bool cond, class Value = void>
struct enable_if_func {
};
template <class Value>
struct enable_if_func<true, Value> {
    using type = Value;
};
template <bool cond, class Value = void>
using enable_if = typename enable_if_func<cond, Value>::type;

// Called std::conditional in the STL
// FIXME? Should we just be consistent here and follow the naming of the STL?
template <bool cond, class TrueValue, class FalseValue>
struct if_else_func {
    using type = FalseValue;
};
template <class TrueValue, class FalseValue>
struct if_else_func<true, TrueValue, FalseValue> {
    using type = TrueValue;
};
template <bool cond, class TrueValue, class FalseValue>
using if_else = typename if_else_func<cond, TrueValue, FalseValue>::type;

template <bool... conds>
constexpr bool all = (conds && ...); // c++17 expansion

template <bool... conds>
constexpr bool any = (conds || ...); // c++17 expansion

template <class FirstValue, class SecondValue>
struct is_same_func : falsey {
};
template <class FirstValue>
struct is_same_func<FirstValue, FirstValue> : truthy {
};
template <class FirstValue, class SecondValue>
constexpr bool is_same = is_same_func<FirstValue, SecondValue>();

template <class Value, class... PossibleValues>
constexpr bool is_one_of = (is_same<Value, PossibleValues> || ...); // c++17 expansion

template <class Value>
constexpr bool is_const = !is_same<remove_const<Value>, Value>;

template <class Value>
constexpr bool is_void = is_same<remove_cv<Value>, void>;

template <class>
struct is_array_func : falsey {
};
template <class Value, size_t size>
struct is_array_func<Value[size]> : truthy {
};
template <class Value>
struct is_array_func<Value[]> : truthy {
};
template <class Value>
constexpr bool is_array = is_array_func<Value>::value;

template <class Value>
constexpr bool is_reference = !is_same<remove_ref<Value>, Value>;

template <typename Value>
struct is_lvalue_impl : falsey {};
template <typename Value>
struct is_lvalue_impl<Value&> : truthy {};

template <class Value>
constexpr bool is_lvalue = is_lvalue_impl<Value>::value;

template <typename Value>
struct is_rvalue_impl : falsey {};
template <typename Value>
struct is_rvalue_impl<Value&&> : truthy {};

template <class Value>
constexpr bool is_rvalue = is_rvalue_impl<Value>::value;

// See https://en.cppreference.com/w/cpp/types/is_function; this is a clever
// and cleaner way to implement is_function than some (boost) implementations:
template <class Value>
constexpr bool is_function = !is_const<const Value> && !is_reference<Value>;

template <class Value>
constexpr bool is_integer = is_one_of<remove_cv<Value>, unsigned long long, long long, unsigned long, long, unsigned int, int, unsigned short, short, signed char, unsigned char, char, bool>;

template <class Value>
constexpr bool is_floating_point = is_one_of<remove_cv<Value>, float, double, long double>;

template <class Value>
constexpr bool is_arithmetic = is_integer<Value> || is_floating_point<Value>;

// Checking if is_same<Value, make_unsigned<Value>> will not work here, if we
// want an e.g. enum class that has an unsigned representation to work.
template <class Value, bool = is_arithmetic<Value>>
struct is_unsigned_impl : true_or_false_func<Value(0) < Value(-1)> {};

template <class Value>
struct is_unsigned_impl<Value, false> : falsey {};

template <class Value>
constexpr bool is_unsigned = is_unsigned_impl<Value>::value;

template <class Value>
constexpr bool is_signed = !is_unsigned<Value>;

template <class>
struct is_pointer_impl : falsey {
};
template <class Value>
struct is_pointer_impl<Value*> : truthy {
};
template <class Value>
struct is_pointer_func : is_pointer_impl<remove_cv<Value>> {
};

template <class Value>
constexpr bool is_pointer = is_pointer_func<Value>::value;

// Here, we use the void_t trick but since Args... prevents a default template,
// parameter we'll kick that responsibility our constexpr bool alias
template <class, class Value, class... Args>
struct is_constructible_func : falsey {
};
template <class Value, class... Args>
// Based on https://en.cppreference.com/w/cpp/types/is_constructible; except
// a new expression is used to prevent against Value being a integral type,
// which causes the expression to be incorrectly treated as a C style cast.
struct is_constructible_func<void_t<decltype(new Value(declval<Args>()...))>, Value, Args...> : truthy {
};

template <class Value, class... Args>
constexpr bool is_constructible = is_constructible_func<void_t<>, Value, Args...>();

// Based on https://en.cppreference.com/w/cpp/types/is_destructible. Of note is
// that for void, function types, and arrays of unknown bounds, this is false.
template <class Value, class = void>
struct is_destructable_func : falsey {
};
template <class Value>
struct is_destructable_func<Value, void_t<decltype(declval<Value&>().~Value())>> : truthy {
};

template <class Value>
constexpr bool is_destructible = is_destructable_func<Value>();

// https://en.cppreference.com/w/cpp/concepts/constructible_from
template <class Value, class... Args>
constexpr bool is_constructible_from = all<is_destructible<Value>, is_constructible<Value, Args...>>;

// https://en.cppreference.com/w/cpp/concepts/convertible
// and https://en.cppreference.com/w/cpp/types/is_convertible
//
// Unlike the STL, we differentiate between implicit and explicit conversion.
// The implicit conversion is called "is_convertible" in the STL and the
// convertible_to concept requires both implicit and explicit conversions
// (which is defined in the requires clause with a static_cast).
namespace impl {

template <class Value>
static void test_conversion(Value);

}

template <class FromValue, class ToValue, class = void>
struct is_implicitly_convertible_func : falsey {
}; // void converts to void
template <class FromValue, class ToValue>
struct is_implicitly_convertible_func<FromValue, ToValue, void_t<decltype(impl::test_conversion<ToValue>(declval<FromValue>()))>> : truthy {
};

// FIXME: Re-evaluate this; not sure if our is_* checks work with references here
template <class FromValue, class ToValue>
constexpr bool is_implicitly_convertible = (is_void<FromValue> && is_void<ToValue>) || // void converts to void
    ((!is_void<FromValue> && !is_array<FromValue> && !is_function<FromValue>)&&is_implicitly_convertible_func<FromValue, ToValue>());

template <class FromValue, class ToValue, class = void>
struct is_explicitly_convertible_func : falsey {
};
template <class FromValue, class ToValue>
struct is_explicitly_convertible_func<FromValue, ToValue, void_t<decltype(static_cast<ToValue>(declval<FromValue>()))>> : truthy {
};

template <class FromValue, class ToValue>
constexpr bool is_explicitly_convertible = is_explicitly_convertible_func<FromValue, ToValue>();

template <class FromValue, class ToValue>
constexpr bool is_convertible_to = is_implicitly_convertible<FromValue, ToValue>&& is_explicitly_convertible<FromValue, ToValue>;

// https://en.cppreference.com/w/cpp/concepts/move_constructible
template <class Value>
constexpr bool is_move_constructible = all<is_constructible_from<Value, Value>,
    is_convertible_to<Value, Value>>;

// Here, we follow the C++20 concepts definition, which includes move
// constructible:
// https://en.cppreference.com/w/cpp/concepts/copy_constructible
template <class Value>
constexpr bool is_copy_constructible = all<is_move_constructible<Value>,
    is_constructible_from<Value, Value&>, is_convertible_to<Value&, Value>,
    is_constructible_from<Value, const Value&>, is_convertible_to<const Value&, Value>,
    is_constructible_from<Value, const Value>, is_convertible_to<const Value, Value>>;

}

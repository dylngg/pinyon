#pragma once

#include <cstddef>
#include <cstdint>
#include <new> // GCC provides this with -ffreestanding

/*
 * Define easier to type aliases for sized integers here.
 */
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

// ARCH: 32-bit dependent
// We do necessary pointer arithmetic that we cannot do with void* unless
// we're ok with the compiler constantly screaming at us.
using PtrData = uintptr_t;

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

template <class Value>
struct remove_ref {
    using type = Value;
};
template <class Value>
struct remove_ref<Value&> {
    using type = Value;
};
template <class Value>
struct remove_ref<Value&&> {
    using type = Value;
};

template <typename Value>
constexpr typename remove_ref<Value>::type&& move(Value&& value)
{
    return static_cast<typename remove_ref<Value>::type&&>(value);
}

template <typename Value>
void swap(Value& first, Value& second)
{
    Value temp = move(first);
    first = move(second);
    second = move(temp);
}

/*
 * Our own std::optional, complete with the arcane placement new operator, and
 * overzelaus use of constexpr in an offer to the compiler gods that this shall
 * be a zero-cost abstraction! (Also, this is usable in constexpr contexts)
 *
 * See https://github.com/akrzemi1/Optional/blob/master/optional.hpp for a
 * reference implementation of std::optional.
 */
template <typename Value>
struct Maybe {
    // empty: e.g. Maybe<Value> maybe {};
    constexpr Maybe() = default;
    constexpr static Maybe Not() { return {}; }

    // ctor: e.g. int i = 0; Maybe<Value> maybe { i };
    constexpr Maybe(const Value& value)
        : m_has_value(true)
    {
        new (&m_value_space) Value(value);
    }

    // rvalue ctor: e.g. Maybe<std::unique_ptr<C>> maybe { std::make_unique<C>() };
    constexpr Maybe(Value&& value)
        : m_has_value(true)
    {
        new (&m_value_space) Value(move(value));
    }

    ~Maybe()
    {
        destroy_value_if_present();
    }

    // copy ctor: e.g. Maybe<Value> other; Maybe<Value> maybe { other };
    constexpr Maybe(const Maybe& other_maybe)
    {
        if ((m_has_value = other_maybe.has_value()))
            new (&m_value_space) Value(other_maybe.value());
    }

    // rvalue ctor: e.g. Maybe<Value> maybe { Maybe<Value> {} };
    constexpr Maybe(Maybe&& other_maybe)
    {
        if ((m_has_value = other_maybe.has_value()))
            new (&m_value_space) Value(other_maybe.release_value());
    }

    // copy assignment: e.g. Maybe<Value> other; Maybe<Value> maybe = other;
    constexpr Maybe& operator=(const Maybe& other_maybe)
    {
        if (this != &other_maybe) {
            destroy_value_if_present();
            if ((m_has_value = other_maybe.has_value()))
                new (&m_value_space) Value(other_maybe.value());
        }
        return *this;
    }

    // move assignment: e.g. Maybe<std::unique_ptr<Value>> maybe = std::move(std::make_unique<Value>());
    constexpr Maybe& operator=(Maybe&& other_maybe)
    {
        if (this != &other_maybe) {
            destroy_value_if_present();
            if ((m_has_value = other_maybe.has_value()))
                new (&m_value_space) Value(other_maybe.release_value());
        }
        return *this;
    }

    [[nodiscard]] constexpr bool has_value() const { return m_has_value; }
    // Allow for if (maybe) ..., rather than if (!maybe.has_value())
    constexpr explicit operator bool() const { return has_value(); }

    constexpr Value&& release() { return release_value(); }
    constexpr const Value&& release() const { return release_value(); }
    constexpr Value& value() { return *value_ptr(); }
    constexpr const Value& value() const { return *value_ptr(); }

    constexpr Value& operator*() { return value(); }
    constexpr const Value& operator*() const { return value(); }
    constexpr Value* operator->() { return value_ptr(); }
    constexpr const Value* operator->() const { return value_ptr(); }

private:
    constexpr Value&& release_value()
    {
        m_has_value = false;
        return move(value());
    }

    constexpr Value* value_ptr()
    {
        return reinterpret_cast<Value*>(&m_value_space);
    }
    constexpr const Value* value_ptr() const
    {
        return reinterpret_cast<const Value*>(&m_value_space);
    }

    constexpr void destroy_value_if_present()
    {
        if (has_value())
            value().~Value();
    }

    alignas(Value) u8 m_value_space[sizeof(Value)];
    bool m_has_value = false;
};

template <class First, class Second>
struct Pair {
    First first;
    Second second;
};

#pragma once

#include <new>

#include "type_traits.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace pine {

/*
 * Our own std::optional, complete with the arcane placement new operator, and
 * overzelaus use of constexpr in an offer to the compiler gods that this shall
 * be a zero-cost abstraction! (Hint: It is not. But it _is_ usable in
 * constexpr contexts)
 *
 * Notably, this std::optional doesn't cover cases where the copy constructor
 * is deleted, and probably a bunch of other C++ corner cases I'm not aware of
 * for that matter.
 *
 * See https://github.com/akrzemi1/Optional/blob/master/optional.hpp for a
 * reference implementation of std::optional.
 */
template <typename Value>
struct Maybe : private ConditionallyCopyable<Value>, private ConditionallyMovable<Value> {
    // empty: e.g. Maybe<Value> maybe {};
    constexpr Maybe()
        : m_has_value(false) {};
    constexpr static Maybe Not() { return {}; }

    // ctor: e.g. int i = 0; Maybe<Value> maybe { i };
    template<class Convert, enable_if<is_implicitly_convertible<Convert, Value>>* = nullptr>
    constexpr Maybe(const Convert& value)
        : m_has_value(true)
    {
        new (&m_value_space) Value(value);
    }
    // Make explicit if the given type can be converted to Value
    // https://devblogs.microsoft.com/cppblog/c20s-conditionally-explicit-constructors/
    template<class Convert, enable_if<!is_implicitly_convertible<Convert, Value>>* = nullptr>
    constexpr explicit Maybe(const Convert& value)
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
            if ((m_has_value = other_maybe.has_value()))
                new (&m_value_space) Value(other_maybe.value());
        }
        return *this;
    }

    // move assignment: e.g. Maybe<std::unique_ptr<Value>> maybe = std::move(std::make_unique<Value>());
    constexpr Maybe& operator=(Maybe&& other_maybe)
    {
        if (this != &other_maybe) {
            destroy_value_if_present(); // rebind
            if ((m_has_value = other_maybe.has_value()))
                new (&m_value_space) Value(other_maybe.release_value());
        }
        return *this;
    }

    [[nodiscard]] constexpr bool has_value() const { return m_has_value; }
    // Allow for if (maybe) ..., rather than if (!maybe.has_value())
    constexpr explicit operator bool() const { return has_value(); }

    constexpr Value release_value()
    {
        m_has_value = false;
        return move(value());
    }
    constexpr Value& value() & { return *value_ptr(); }
    constexpr const Value& value() const& { return *value_ptr(); }
    constexpr Value value() && { return release_value(); }

    constexpr Value& operator*() & { return value(); }
    constexpr const Value& operator*() const& { return value(); }
    constexpr Value operator*() && { return release_value(); }

    // Typically you return Value* for these instead of Value&; however we want
    // forwarding -> operators, such that if the Value we have also overloads
    // the -> operator, that operator will be applied.
    constexpr Value& operator->() & { return value(); }
    constexpr const Value& operator->() const& { return value(); }

private:
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

}

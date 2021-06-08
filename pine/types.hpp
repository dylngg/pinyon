#pragma once

#include <cstddef>
#include <cstdint>
#include <new> // GCC provides this with -ffreestanding

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
using PtrData = u32;

/*
 * Our own std::optional, complete with the arcane placement new operator, and
 * judicious use of constexpr and __inline__ in an offer to the compiler gods
 * that this shall be a zero-cost abstraction!
 *
 * See https://github.com/akrzemi1/Optional/blob/master/optional.hpp for a
 * reference implementation of std::optional.
 */
template <typename Value>
struct Maybe {
    constexpr Maybe()
        : m_value_space()
        , m_has_value(false) {};
    constexpr Maybe(const Value& value)
        : m_has_value(true)
    {
        new (&m_value_space) Value(value);
    }
    constexpr Maybe(const Maybe& other_maybe)
        : m_has_value(other_maybe.m_has_value)
    {
        if (m_has_value)
            new (&m_value_space) Value(other_maybe.value());
    }
    constexpr Maybe& operator=(const Maybe& other_maybe)
    {
        if (this != &other_maybe) {
            value().~Value();
            if (other_maybe.m_has_value)
                new (&m_value_space) Value(other_maybe.value());
            m_has_value = other_maybe.m_has_value;
        }
        return *this;
    }
    ~Maybe()
    {
        if (m_has_value)
            value().~Value();
    }

    constexpr bool has_value() const { return m_has_value; }
    constexpr explicit operator bool() const
    {
        /* Allow for if (maybe) ..., rather than if (!maybe.has_value()) */
        return has_value();
    }

    constexpr const Value& value() const { return *reinterpret_cast<const Value*>(&m_value_space); }
    inline Value& value() { return *reinterpret_cast<Value*>(&m_value_space); }
    constexpr static Maybe<Value> No() { return Maybe<Value>(); }

private:
    alignas(Value) u8 m_value_space[sizeof(Value)];
    bool m_has_value;
};

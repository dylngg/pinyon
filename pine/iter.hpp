#pragma once
#include "metaprogramming.hpp"
#include "twomath.hpp"
#include "types.hpp"

namespace pine {

/*
 * The type of iterator; should be accessible with the 'type' static variable
 * in the iterator.
 */
enum class IterType {
    Output = 0b1,               // can be written to
    Input = 0b10,               // can be read from
    IO = 0b11,                  // can be read and written to
    Forward = 0b110,            // can be read from and supports multiple passes
    Bidirectional = 0b1110,     // can be read from, supports multiple passes and movement backwards
    RandomAccess = 0b11110,     // can be read from, supports multiple passes, movement backwards, and random access
};

inline const char* to_static_string(IterType iter_type)
{
    switch (iter_type) {
    case IterType::Input:
        return "IterType::Input";
    case IterType::Output:
        return "IterType::Output";
    case IterType::IO:
        return "IterType::IO";
    case IterType::Forward:
        return "IterType::Forward";
    case IterType::Bidirectional:
        return "IterType::Bidirectional";
    case IterType::RandomAccess:
        return "IterType::RandomAccess";
    }
    return "IterType::<Unknown>!";
}

constexpr unsigned iter_type_mask(IterType type)
{
    return 1u << (bit_width(static_cast<unsigned>(type)) - 1);
}

template <typename Iter, IterType, typename = void>
struct is_iter_type_func : falsey {};
template <typename Iter, IterType type>
struct is_iter_type_func<Iter, type, void_t<decltype(Iter::type)>> : true_or_false_func<(static_cast<unsigned>(Iter::type) & iter_type_mask(type)) != 0> {};

template <typename Iter, IterType, typename = void>
struct has_iter_trait_func : falsey {};
template <typename Iter, IterType trait>
struct has_iter_trait_func<Iter, trait, void_t<decltype(Iter::type)>> : true_or_false_func<(static_cast<unsigned>(Iter::type) & static_cast<unsigned>(trait)) != 0> {};


template <typename Iter>
constexpr bool is_random_access_iter = is_iter_type_func<Iter, IterType::RandomAccess>::value;

template <typename Iter>
constexpr bool is_bidirectional_iter = is_iter_type_func<Iter, IterType::Bidirectional>::value;

template <typename Iter>
constexpr bool is_forward_iter = is_iter_type_func<Iter, IterType::Forward>::value;

template <typename Iter>
constexpr bool is_output_iter = has_iter_trait_func<Iter, IterType::Output>::value;

template <typename Iter, typename Distance, enable_if<is_random_access_iter<Iter>, Iter*> = nullptr>
void advance(Iter& it, Distance steps)
{
    it += steps;
}

template <typename Iter, typename Distance, enable_if<!is_random_access_iter<Iter> && is_bidirectional_iter<Iter>, void*> = nullptr>
void advance(Iter& it, Distance steps)
{
    while (steps > 0) {
        ++it;
        --steps;
    }
    while (steps < 0) {
        --it;
        ++steps;
    }
}

template <typename Iter, typename Distance, enable_if<!is_random_access_iter<Iter> && !is_bidirectional_iter<Iter>, void*> = nullptr>
void advance(Iter& it, Distance steps)
{
    while (steps > 0) {
        ++it;
        --steps;
    }
}

template <typename Iter>
Iter next(Iter it, size_t steps = 1)
{
    advance(it, steps);
    return it;
}

template <typename Iter>
Iter prev(Iter it, size_t steps = 1)
{
    advance(it, -steps);
    return it;
}

template <typename Iter, enable_if<is_random_access_iter<Iter>>* = nullptr>
size_t distance(Iter begin, Iter end)
{
    return end - begin;
}

template <typename Iter, enable_if<!is_random_access_iter<Iter>>* = nullptr>
size_t distance(Iter begin, Iter end)
{
    size_t dist = 0;
    while (begin != end)
        ++begin;
    return dist;
}

/*
 * The different iterators defined here fit different container patterns.
 *
 * In order for a container to use an iterator here, they should simply return
 * an Iter<Container, Value>::begin(...) and Iter<Container, Value>::end(...)
 * in their begin() and end() methods and fufill the particular iterator
 * requirements (such as having a .length() and operator[] method).
 *
 * This works because we simply make each container (typename Wraps) our friend
 * in each iterator and template duck-type our way into things working.
 *
 * Inspired by SerenityOS's SimpleIterator in their AK library.
 */

/*
 * Iterator for Wraps classes with random access to some Value. Requires
 * that Wraps implements a .length() and operator[] method.
 *
 * The iterator is guaranteed to be stable, so long as the length is not
 * reduced below the corresponding index of this iterator.
 */
template <typename Wraps, typename Value>
class RandomAccessIter {
public:
    RandomAccessIter() = default;  // required for e.g. default initialization in array[]

    constexpr bool operator==(RandomAccessIter other_iter) const { return m_pos == other_iter.m_pos; }
    constexpr bool operator!=(RandomAccessIter other_iter) const { return m_pos != other_iter.m_pos; }
    RandomAccessIter operator+(size_t offset) { return { *m_wraps, m_pos + offset }; }
    RandomAccessIter& operator++() // prefix increment
    {
        m_pos++;
        return *this;
    }
    RandomAccessIter operator++(int) // postfix increment
    {
        auto prev = *this;
        m_pos++;
        return prev;
    }
    RandomAccessIter& operator+=(size_t offset)
    {
        m_pos += offset;
        return *this;
    }
    size_t operator-(RandomAccessIter& other) { return m_pos - other.m_pos; }
    RandomAccessIter& operator--() // prefix increment
    {
        m_pos--;
        return *this;
    }
    RandomAccessIter operator--(int) // postfix increment
    {
        auto prev = *this;
        m_pos--;
        return prev;
    }
    RandomAccessIter& operator-=(size_t offset)
    {
        m_pos -= offset;
        return *this;
    }
    Value& operator[](int index)
    {
        return (*m_wraps)[index];
    };
    const Value& operator[](int index) const
    {
        return (*m_wraps)[index];
    };
    constexpr Value& operator*() { return (*m_wraps)[m_pos]; };
    constexpr const Value& operator*() const { return (*m_wraps)[m_pos]; };

    constexpr bool at_end() const { return m_pos >= m_wraps->length(); }

    static constexpr IterType type = IterType::RandomAccess;

private:
    RandomAccessIter(Wraps& wraps, size_t pos)
        : m_wraps(&wraps)
        , m_pos(pos)
    {
    }
    friend Wraps;

    // The Wraps class' .begin() and .end() return iterators, so these are
    // there from them
    static RandomAccessIter begin(Wraps& wraps) { return { wraps, 0 }; }
    static RandomAccessIter end(Wraps& wraps) { return { wraps, wraps.length() }; }

    Wraps* m_wraps = nullptr;
    size_t m_pos = 0;
};

/*
 * Iterator for Wraps classes with pointer (next, prev) access. Requires
 * that *Ptr has a .next() and .prev() method.
 *
 * Dereferencing the iterator returns the raw wrapped pointer.
 */
template <typename Wraps, typename Ptr>
class PtrIter {
public:
    constexpr bool operator==(const PtrIter& other) const { return m_ptr == other.m_ptr && m_at_end == other.m_at_end; }
    constexpr bool operator!=(const PtrIter& other) const { return !(*this == other); }

    PtrIter& operator++() // prefix increment
    {
        auto next = m_ptr->next();
        if (next)
            m_ptr = next;
        else
            m_at_end = true;

        return *this;
    }
    PtrIter operator++(int) // postfix increment
    {
        auto iter = *this;
        ++(*this);
        return iter;
    }
    PtrIter& operator--() // prefix increment
    {
        if (m_at_end)
            m_at_end = false;
        else
            m_ptr = m_ptr->prev();

        return *this;
    }
    PtrIter operator--(int) // postfix increment
    {
        auto iter = *this;
        --(*this);
        return iter;
    }
    constexpr Ptr& operator*() { return m_ptr; };
    constexpr const Ptr& operator*() const { return m_ptr; };

    constexpr bool at_end() const { return m_at_end; }

    static constexpr auto type = IterType::Bidirectional;
    using Value = Ptr&;

private:
    struct EndTag {};
    PtrIter(Ptr ptr, Ptr last_ptr)
        : m_ptr(ptr)
        , m_at_end(last_ptr == nullptr)
    {
    }
    friend Wraps;

    // The Wraps class' .begin() and .end() return iterators, so these are
    // there from them
    static PtrIter begin(Ptr ptr, Ptr last_ptr) { return { ptr, last_ptr }; }
    static PtrIter end(Ptr last_ptr) { return { last_ptr, nullptr }; }

    Ptr m_ptr;
    bool m_at_end;
};

}

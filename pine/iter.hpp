#pragma once
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

template <typename Iter>
constexpr bool is_random_access_iter = static_cast<unsigned>(Iter::type) & (1u << (bit_width(static_cast<unsigned>(IterType::RandomAccess)) - 1));

template <typename Iter>
constexpr bool is_bidirectional_iter = static_cast<unsigned>(Iter::type) & (1u << (bit_width(static_cast<unsigned>(IterType::Bidirectional)) - 1));

template <typename Iter>
constexpr bool is_forward_iter = static_cast<unsigned>(Iter::type) & (1u << (bit_width(static_cast<unsigned>(IterType::Forward)) - 1));

template <typename Iter>
constexpr bool is_output_iter = static_cast<unsigned>(Iter::type) & static_cast<unsigned>(Iter::Output);

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
 * Iterator for Wraps classes with sequential access to some Value. Requires
 * that Wraps implements a .length() and operator[] method.
 */
template <typename Wraps, typename Value>
class RandomAccessIter {
public:
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

    Wraps* m_wraps;
    size_t m_pos;
};

}

#pragma once
#include "types.hpp"

namespace pine {

/*
 * The different iterators defined here fit different container patterns.
 *
 * In order for a container to use an iterator here, they should simply return
 * an Iter<Container, Value>::begin(*this) and Iter<Container, Value>::end(*this)
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
class SeqIter {
public:
    constexpr bool operator==(SeqIter other_iter) const { return m_pos == other_iter.m_pos; }
    constexpr bool operator!=(SeqIter other_iter) const { return m_pos != other_iter.m_pos; }
    SeqIter operator+(size_t offset) { return { m_wraps, m_pos + offset }; }
    SeqIter& operator++() // prefix increment
    {
        m_pos++;
        return *this;
    }
    SeqIter operator++(int) // postfix increment
    {
        auto prev = *this;
        m_pos++;
        return prev;
    }
    constexpr Value& operator*() { return m_wraps[m_pos]; };
    constexpr const Value& operator*() const { return m_wraps[m_pos]; };

    constexpr bool at_end() const { return m_pos >= m_wraps.length(); }

private:
    SeqIter(Wraps& wraps, size_t pos)
        : m_wraps(wraps)
        , m_pos(pos)
    {
    }
    friend Wraps;

    // The Wraps class' .begin() and .end() return iterators, so they are
    // there from them
    static SeqIter begin(Wraps& wraps) { return { wraps, 0 }; }
    static SeqIter end(Wraps& wraps) { return { wraps, wraps.length() }; }

    Wraps& m_wraps;
    size_t m_pos;
};

}

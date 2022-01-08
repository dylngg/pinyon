#pragma once
#include "types.hpp"

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
        m_pos++;
        return *this;
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

/*
 * Iterator for Wraps classes with pointers where the first pointer also marks
 * the end of the container. Requires that Ptr implements a .next() function;
 * that also returns the first pointer at the end of the container.
 *
 * The given pointer in begin() and end() may be nullptr, in which case the
 * iterator is empty.
 */
template <typename Wraps, typename Ptr>
class CircularPtrIter {
public:
    constexpr bool operator==(CircularPtrIter other_iter) const { return m_first_ptr == other_iter.m_first_ptr && m_ptr == other_iter.m_ptr; }
    constexpr bool operator!=(CircularPtrIter other_iter) const { return !(*this == other_iter); }
    CircularPtrIter& operator++() // prefix increment
    {
        if (!m_first_ptr)
            m_first_ptr = m_ptr;

        m_ptr = m_ptr->next();
        return *this;
    }
    CircularPtrIter operator++(int) // postfix increment
    {
        if (!m_first_ptr)
            m_first_ptr = m_ptr;

        m_ptr = m_ptr->next();
        return *this;
    }
    constexpr Ptr*& operator*() { return m_ptr; };
    constexpr const Ptr*& operator*() const { return m_ptr; };

    constexpr bool at_end() const
    {
        if (!m_ptr)
            return true;

        return m_first_ptr != nullptr && m_ptr == m_first_ptr;
    }

private:
    CircularPtrIter(Ptr* ptr, Ptr* first_ptr)
        : m_ptr(ptr)
        , m_first_ptr(first_ptr) {};
    friend Wraps;

    // Initially store the first pointer in the current pointer slot but leave
    // the first pointer slot null so we can determine whether we are at the
    // start or the end. When we are incremented, we move the current pointer
    // to the right slot.
    static CircularPtrIter begin(Ptr* first_ptr) { return { first_ptr, nullptr }; }
    static CircularPtrIter end(Ptr* first_ptr) { return { first_ptr, first_ptr }; }

    Ptr* m_ptr;
    Ptr* m_first_ptr;
};
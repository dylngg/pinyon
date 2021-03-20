#pragma once
#include <pine/types.hpp>

// The idea here is to have classes return a specialization of this FwdIter
// template in begin() and end(). As such, initialization code is private
// (and Wraps is allowed because we make them friends here). The Wraps type
// must have a [] operator, .length() function.
// Inspired by SerenityOS's SimpleIterator in their AK library.
template <typename Wraps, typename Value>
class FwdIter {
public:
    constexpr bool operator==(FwdIter other_iter) const { return m_pos == other_iter.m_pos; }
    constexpr bool operator!=(FwdIter other_iter) const { return m_pos != other_iter.m_pos; }
    FwdIter operator+(size_t offset) { return { m_wraps, m_pos + offset }; }
    FwdIter& operator++() // prefix increment
    {
        m_pos++;
        return *this;
    }
    FwdIter operator++(int) // postfix increment
    {
        m_pos++;
        return *this;
    }
    inline constexpr const Value& operator*() const { return m_wraps[m_pos]; };

    constexpr bool at_end() const { return m_pos >= m_wraps.length(); }

    friend Wraps;

private:
    FwdIter(Wraps& wraps, size_t pos)
        : m_wraps(wraps)
        , m_pos(pos)
    {
    }

    // The Wraps class' .begin() and .end() return iterators, so they are
    // there from them
    static FwdIter begin(Wraps& wraps) { return { wraps, 0 }; }
    static FwdIter end(Wraps& wraps) { return { wraps, wraps.length() }; }

    Wraps& m_wraps;
    size_t m_pos;
};
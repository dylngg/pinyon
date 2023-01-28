#pragma once
#include "iter.hpp"
#include "metaprogramming.hpp"
#include "string.hpp"
#include "types.hpp"

namespace pine {

struct StringView {
public:
    StringView(const char* string)
        : m_chars(string)
        , m_length(strlen(string))
    {
    }

    bool operator==(const StringView& other) const
    {
        if (m_length != other.m_length)
            return false;
        return strcmp(m_chars, other.m_chars) == 0;
    }
    const char& operator[](size_t pos) const
    {
        static char dummy = '\0';
        // FIXME: I'd rather have an assert here than this wackyness; this will just
        //        hide bugs
        if (pos >= m_length)
            return dummy;
        return m_chars[pos];
    }

    using ConstIter = RandomAccessIter<const StringView, const char>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    size_t length() const { return m_length; }
    const char* data() const { return m_chars; }

private:
    const char* m_chars;
    const size_t m_length;
};

}

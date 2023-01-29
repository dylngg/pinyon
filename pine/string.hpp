#pragma once

#include "c_string.hpp"
#include "iter.hpp"
#include "memory.hpp"
#include "utility.hpp"
#include "string_view.hpp"

namespace pine {

template <typename Allocator, typename Destructor = DefaultDestructor<char, Allocator>>
class String : private Destructor {
public:
    String(Allocator& allocator)
        : Destructor(allocator) {};
    static Maybe<String> try_create(Allocator& allocator, const char* from)
    {
        size_t length = strlen(from);
        auto [ptr, _] = allocator.allocate(length);
        if (!ptr)
            return {};

        auto* dest = static_cast<char*>(ptr);
        strcopy(dest, from);
        return String(allocator, dest, length);
    }
    ~String()
    {
        if (m_string)
            Destructor::destroy(m_string);
    }
    String(const String& other) = delete;
    String(String&& other)
        : Destructor(other.allocator())
        , m_length(exchange(other.m_length, 0u))
        , m_string(exchange(other.m_string, nullptr)) {};

    String& operator=(String other)
    {
        swap(*this, other);
        return *this;
    }

    /*
     * Returns the length of the string in bytes.
     */
    size_t length() const { return m_length; }

    /*
     * Checks whether the string is empty.
     */
    bool empty() const { return m_length == 0; }

    char& operator[](size_t index)
    {
        return m_string[index];
    }
    const char& operator[](size_t index) const
    {
        return m_string[index];
    }
    bool operator==(StringView other) const
    {
        return strcmp(m_string, other.data()) == 0;
    }

    operator StringView() const
    {
        return StringView(m_string, m_length);
    }

    using Iter = RandomAccessIter<String<Allocator, Destructor>, char>;
    Iter begin() { return Iter::begin(*this); }
    Iter end() { return Iter::end(*this); }

    using ConstIter = RandomAccessIter<const String<Allocator, Destructor>, const char>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    friend void swap(String& first, String& second)
    {
        using pine::swap;

        swap(first.m_length, second.m_length);
        swap(first.m_string, second.m_string);
    }

private:
    String(Allocator& allocator, char* string, size_t length)
        : Destructor(allocator)
        , m_length(length)
        , m_string(string) {};

    size_t m_length = 0;
    char* m_string = nullptr;
};

}

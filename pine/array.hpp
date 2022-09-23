#pragma once

#include <initializer_list>  // comes with -ffreestanding

#include <pine/iter.hpp>
#include <pine/types.hpp>
#include <pine/utility.hpp>

namespace pine {

template <typename Value, size_t Size>
class Array {
public:
    Array() = default;
    Array(std::initializer_list<Value> items)
    {
        size_t i = 0;
        for (auto item : items)
            m_contents[i++] = pine::move(item);
    }

    /*
     * Returns the number of entries.
     */
    [[nodiscard]] constexpr size_t length() const { return Size; }

    void insert(size_t index, Value&& value)
    {
        m_contents[index] = Value(pine::move(value));
    }

    Value remove(size_t index)
    {
        return pine::move(m_contents[index]);
    }
    [[nodiscard]] Value& operator[](size_t index) &
    {
        return m_contents[index];
    }
    [[nodiscard]] const Value& operator[](size_t index) const&
    {
        return m_contents[index];
    }

    using Iter = RandomAccessIter<Array<Value, Size>, Value>;
    Iter begin() { return Iter::begin(*this); }
    Iter end() { return Iter::end(*this); }

    using ConstIter = RandomAccessIter<const Array<Value, Size>, const Value>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    friend void swap(Array& first, Array& second)
    {
        using pine::swap;

        swap(first.m_contents, second.m_contents);
    }

private:
    Value m_contents[Size];
};

}

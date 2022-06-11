#pragma once

#include <initializer_list>  // comes with -ffreestanding

#include <pine/math.hpp>
#include <pine/twomath.hpp>
#include <pine/types.hpp>
#include <pine/units.hpp>
#include <pine/utility.hpp>

namespace pine {

template <typename Value, typename Allocator>
class Vector {
public:
    Vector() = default;
    Vector(std::initializer_list<Value> values)
        : Vector()
    {
        if (!ensure(values.size()))
            return;

        for (auto value : values)
            emplace_unensured(move(value));
    };
    ~Vector()
    {
        if (m_contents)
            Allocator::allocator().free(m_contents);
    }
    Vector(const Vector& other)
        : m_count()
        , m_capacity()
        , m_contents()
    {
        if (!ensure(other.length()))
            return;

        for (auto value : other)
            emplace_unensured(value);
    }
    Vector(Vector&& other)
        : m_count(exchange(other.m_count, 0u))
        , m_capacity(exchange(other.m_capacity, 0u))
        , m_contents(exchange(other.m_contents, nullptr)) {}

    Vector& operator=(Vector other)
    {
        swap(*this, other);
        return *this;
    }

    /*
     * Increases the internal storage to fit the required amount of entries,
     * if needed. Returns whether the increase was successful (whether memory
     * was able to be allocated if required).
     */
    [[nodiscard]] bool ensure(size_t amount)
    {
        if (amount <= m_capacity)
            return true;

        size_t new_capacity = align_capacity(amount);

        auto alloc = Allocator::allocator();
        auto [ptr, allocated_size] = alloc.allocate(new_capacity * sizeof(Value));
        auto* old_contents = m_contents;
        m_contents = static_cast<Value*>(ptr);
        if (!m_contents)
            return false;

        new_capacity = allocated_size / sizeof(Value);  // allocator may return more; use full space
        if (old_contents) {
            memcpy(m_contents, old_contents, m_count * sizeof(Value));
            alloc.free(old_contents);
        }
        m_capacity = new_capacity;
        return true;
    }

    /*
     * Returns the number of entries.
     */
    size_t length() const { return m_count; }

    /*
     * Returns the total capacity for entries.
     */
    size_t capacity() const { return m_capacity; }

    /*
     * Checks whether the container has no entries.
     */
    bool empty() const { return m_count == 0; }

    /*
     * Attempts to append an item to the vector and returns whether the append
     * was successful (whether memory was able to be allocated if required).
     */
    bool append(const Value& value)
    {
        if (!ensure(m_count+1))
            return false;

        emplace_unensured(value);
        return true;
    }

    Value& operator[](size_t index)
    {
        return m_contents[index];
    }
    const Value& operator[](size_t index) const
    {
        return m_contents[index];
    }

    using Iter = RandomAccessIter<Vector<Value, Allocator>, Value>;
    Iter begin() { return Iter::begin(*this); }
    Iter end() { return Iter::end(*this); }

    using ConstIter = RandomAccessIter<const Vector<Value, Allocator>, const Value>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    friend void swap(Vector& first, Vector& second)
    {
        using pine::swap;

        swap(first.m_count, second.m_count);
        swap(first.m_capacity, second.m_capacity);
        swap(first.m_contents, second.m_contents);
    }

private:
    template <typename... Args>
    void emplace_unensured(Args&&... args)
    {
        new (&m_contents[m_count]) Value(forward<Args>(args)...);
        ++m_count;
    }

    size_t align_capacity(size_t new_capacity) const
    {
        // Normally vectors will double their capacity when out of space; we
        // do the same here, except when we get past a page size, since our
        // potential waste increases substantially
        if (new_capacity > PageSize)
            return align_up_two(new_capacity, PageSize);

        // align_up_to_power cannot be called with a number greater than 1 << (limits<...>::width - 1)
        return max(align_up_to_power(new_capacity), (size_t)8u);
    }

    size_t m_count = 0;
    size_t m_capacity = 0;
    Value* m_contents = nullptr;
};

}

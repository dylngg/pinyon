#pragma once

#include <initializer_list>  // comes with -ffreestanding

#include <pine/algorithm.hpp>
#include <pine/math.hpp>
#include <pine/page.hpp>
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

        return resize(amount);
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
    bool append(Value&& value)
    {
        if (!ensure(m_count+1))
            return false;

        emplace_unensured(pine::move(value));
        return true;
    }

    /*
     * Removes an item at the given index.
     */
    bool remove(size_t index)
    {
        if (index >= m_count)
            return false;

        m_contents[index].Value::~Value();
        m_count--;

        if (index == m_count)  // tail; no need for resize
            return true;

        size_t new_capacity = align_capacity(m_count);

        auto& alloc = Allocator::allocator();
        auto [ptr, new_size] = alloc.allocate(new_capacity * sizeof(Value));
        auto new_contents = static_cast<Value*>(ptr);
        if (!new_contents)
            return false;

        size_t from = 0;
        size_t to = 0;
        while (to < m_count) {
            if (from == index) {
                from++;
                continue;
            }

            new (new_contents + to) Value(move(*(m_contents + from)));
            from++;
            to++;
        }

        m_capacity = new_size;
        auto old_contents = m_contents;
        m_contents = new_contents;
        if (old_contents) {
            auto& alloc = Allocator::allocator();
            alloc.free(old_contents);
        }

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

    // Resizes the capacity to be amount and returns whether it was successful
    // in doing so. No destructors are ran if the new amount is less than the
    // old capacity
    bool resize(size_t amount)
    {
        size_t new_capacity = align_capacity(amount);

        auto& alloc = Allocator::allocator();
        auto [ptr, new_size] = alloc.allocate(new_capacity * sizeof(Value));
        auto new_contents = static_cast<Value*>(ptr);
        if (!new_contents)
            return false;

        for (size_t i = 0; i < m_count; i++)
            new (new_contents + i) Value(move(*(m_contents + i)));

        m_capacity = new_size;
        auto old_contents = m_contents;
        m_contents = new_contents;
        if (old_contents)
            alloc.free(old_contents);

        return true;
    }

    size_t m_count = 0;
    size_t m_capacity = 0;
    Value* m_contents = nullptr;
};

}

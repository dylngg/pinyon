#pragma once
#include "metaprogramming.hpp"
#include "utility.hpp"

/*
 * Our own unique_ptr, except we are owning a reference to an object, not a
 * pointer. Thus, Owner<> is never null.
 */
template <class Value, class Destructor>
class Owner {
public:
    Owner(enable_if<is_move_constructible<Value>, Value>& value)
        : m_ptr(&value) {};
    ~Owner()
    {
        if (m_ptr) {
            Destructor d {};
            d(m_ptr);
        }
    }

    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;

    Owner(Owner&& other)
        : m_ptr(exchange(other.m_ptr, nullptr)) {};
    Owner& operator=(Owner&& other)
    {
        Owner owner { move(other) };
        swap(*this, owner);
        return *this;
    }

    Value* get() { return m_ptr; }
    const Value* get() const { return m_ptr; }

    Value* operator->() { return get(); }
    const Value* operator->() const { return get(); }
    Value& operator*() { return *get(); }
    const Value& operator*() const { return *get(); }

    friend void swap(Owner& first, Owner& second)
    {
        swap(first.m_ptr, second.m_ptr);
    }

private:
    Value* m_ptr;
};
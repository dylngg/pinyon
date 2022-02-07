#pragma once
#include "maybe.hpp"
#include "metaprogramming.hpp"
#include "utility.hpp"

template <class Value, class Allocator>
struct DefaultDestructor {
    constexpr DefaultDestructor(Allocator& allocator = Allocator::allocator())
        : m_allocator(allocator) {};
    constexpr void operator()(Value* value)
    {
        value->~Value();
        m_allocator.free(value);
    };

private:
    Allocator& m_allocator;
};

/*
 * Our own unique_ptr, except we are owning a reference to an object, not a
 * pointer. Thus, Owner<> is never null unless moved.
 */
template <class Value, class Allocator, class Destructor = DefaultDestructor<Value, Allocator>>
class Owner {
public:
    using ValueOwner = Owner<Value, Allocator, Destructor>;

    Owner(enable_if<is_move_constructible<Value>, Value>& value)
        : m_ptr(&value) {};
    ~Owner()
    {
        if (m_ptr) {
            Destructor d {};
            d(m_ptr);
        }
    }

    template <class... Args>
    static Maybe<ValueOwner> try_create(Args&&... args)
    {
        void* ptr = Allocator::allocator().allocate(sizeof(Value));
        if (!ptr)
            return {};

        auto* value = new (ptr) Value(forward<Args>(args)...);
        return { *value };
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
        ::swap(first.m_ptr, second.m_ptr);
    }

private:
    Value* m_ptr;
};

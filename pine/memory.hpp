#pragma once
#include "maybe.hpp"
#include "metaprogramming.hpp"
#include "utility.hpp"

namespace pine {

template <class Value, class Allocator>
class DefaultDestructor {
public:
    Allocator& allocator() const
    {
        return *m_allocator;
    };

protected:
    constexpr explicit DefaultDestructor(Allocator& allocator)
        : m_allocator(&allocator) {};

    constexpr void destroy(Value* value)
    {
        value->~Value();
        m_allocator->free(pine::Allocation{ value, sizeof(Value) });
    };

private:
    mutable Allocator* m_allocator;
};

/*
 * Our own unique_ptr, except we are owning a reference to an object, not a
 * pointer. Thus, Owner<> is never null unless moved.
 */
template <class Value, class Allocator, class Destructor = DefaultDestructor<Value, Allocator>>
class Owner : public Destructor {
public:
    using ValueOwner = Owner<Value, Allocator, Destructor>;

    Owner(Allocator& allocator, Value& value)
        : Destructor(allocator)
        , m_ptr(&value) {};
    ~Owner()
    {
        if (m_ptr) {
            Destructor::destroy(m_ptr);
        }
    }

    template <class... Args>
    static Maybe<ValueOwner> try_create(Allocator& allocator, Args&&... args)
    {
        auto [ptr, _] = allocator.allocate(sizeof(Value));
        if (!ptr)
            return {};

        auto* value = new (ptr) Value(forward<Args>(args)...);
        return { ValueOwner(allocator, *value) };
    }

    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;

    Owner(Owner&& other)
        : Destructor(other.allocator())
        , m_ptr(exchange(other.m_ptr, nullptr)) {}

    template <typename Other, typename OtherDestructor>
    Owner(Owner<Other, Allocator, OtherDestructor>&& other)
        : Destructor(other.allocator())
        , m_ptr(other.release()) {}

    Owner& operator=(Owner&& other)
    {
        Owner owner { move(other) };
        swap(*this, owner);
        return *this;
    }

    Value* get() { return m_ptr; }
    const Value* get() const { return m_ptr; }

    Value* release() { return exchange(m_ptr, nullptr); }

    Value* operator->() { return get(); }
    const Value* operator->() const { return get(); }
    Value& operator*() { return *get(); }
    const Value& operator*() const { return *get(); }

    friend void swap(Owner& first, Owner& second)
    {
        pine::swap(first.m_ptr, second.m_ptr);
    }

private:
    Value* m_ptr;
};

}

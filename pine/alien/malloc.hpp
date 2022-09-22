#pragma once

#include <pine/malloc.hpp>

#include <cstdlib>
#include <cassert>

namespace alien {

class Allocator {
public:
    static Allocator& allocator()
    {
        static Allocator allocator {};
        return allocator;
    }

    Pair<void*, size_t> allocate(size_t requested_size)
    {
        auto* ptr = std::malloc(requested_size);
        return { ptr, ptr ? requested_size : 0 };
    }
    void free(pine::Allocation alloc) { std::free(alloc.ptr); }
};

inline pine::Allocation malloc(size_t requested_size)
{
    return Allocator::allocator().allocate(requested_size);
}

template <typename Value>
inline Value* malloc()
{
    auto [ptr, size] = malloc(sizeof(Value));
    return static_cast<Value*>(ptr);
}

inline void free(pine::Allocation alloc)
{
    Allocator::allocator().free(alloc);
}

template <typename Value>
inline void free(Value* ptr)
{
    free(pine::Allocation(ptr, sizeof(Value)));
}

}

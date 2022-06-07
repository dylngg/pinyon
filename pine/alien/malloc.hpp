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
    void free(void* ptr) { std::free(ptr); }
    bool in_bounds(void*) const { return true; }
};

inline void* malloc(size_t requested_size)
{
    auto [ptr, alloc_size] = Allocator::allocator().allocate(requested_size);
    return ptr;
}

template <typename Value>
inline Value* malloc()
{
    return static_cast<Value*>(malloc(sizeof(Value)));
}

inline void free(void* ptr)
{
    Allocator::allocator().free(ptr);
}

template <typename Value>
inline void free(Value* ptr)
{
    free(ptr);
}

}

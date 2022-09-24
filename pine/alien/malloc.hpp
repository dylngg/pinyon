#pragma once

#include <pine/malloc.hpp>

#include <cstdlib>
#include <cassert>

namespace alien {

class Allocator {
public:
    pine::Allocation allocate(size_t requested_size)
    {
        auto* ptr = std::malloc(requested_size);
        return { ptr, ptr ? requested_size : 0 };
    }
    void free(pine::Allocation alloc) { std::free(alloc.ptr); }
};

static Allocator& allocator()
{
    static Allocator allocator {};
    return allocator;
}


inline pine::Allocation malloc(size_t requested_size)
{
    return allocator().allocate(requested_size);
}

template <typename Value>
inline Value* malloc()
{
    auto [ptr, size] = malloc(sizeof(Value));
    return static_cast<Value*>(ptr);
}

inline void free(pine::Allocation alloc)
{
    allocator().free(alloc);
}

template <typename Value>
inline void free(Value* ptr)
{
    free(pine::Allocation(ptr, sizeof(Value)));
}

}

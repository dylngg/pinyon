#pragma once

#include "kmalloc.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

class Stack {
public:
    static Maybe<Stack> try_create(size_t size);
    u32 sp() const;

private:
    Stack(KOwner<u32> base, size_t size)
        : m_size(size)
        , m_bottom(move(base)) {};

    size_t m_size;
    KOwner<u32> m_bottom;
};


#pragma once

#include "kmalloc.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

class Stack {
public:
    static pine::Maybe<Stack> try_create(size_t size);
    PtrData sp() const;

private:
    Stack(KOwner<PtrData> base, size_t size)
        : m_size(size)
        , m_bottom(pine::move(base)) {};

    size_t m_size;
    KOwner<PtrData> m_bottom;
};


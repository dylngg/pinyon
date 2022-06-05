#include "stack.hpp"

pine::Maybe<Stack> Stack::try_create(size_t size)
{
    size_t allocated_size = pine::align_up_two(size, Page);
    u32* stack_ptr = static_cast<u32*>(kmalloc(allocated_size));
    if (!stack_ptr)
        return {};

    return Stack { *stack_ptr, allocated_size };
}

u32 Stack::sp() const
{
    return reinterpret_cast<PtrData>(m_bottom.get()) + m_size;
}



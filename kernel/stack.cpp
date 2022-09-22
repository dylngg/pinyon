#include "stack.hpp"

pine::Maybe<Stack> Stack::try_create(size_t size)
{
    size_t allocated_size = pine::align_up_two(size, PageSize);
    auto stack_alloc = kmalloc(allocated_size);
    if (!stack_alloc)
        return {};

    u32* stack_ptr = static_cast<u32*>(stack_alloc.ptr);
    return Stack { KOwner<u32>(kernel_allocator(), *stack_ptr), allocated_size };
}

u32 Stack::sp() const
{
    return reinterpret_cast<PtrData>(m_bottom.get()) + m_size;
}



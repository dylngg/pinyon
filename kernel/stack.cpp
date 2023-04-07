#include "stack.hpp"

pine::Maybe<Stack> Stack::try_create(size_t size)
{
    size_t allocated_size = pine::align_up_two(size, PageSize);
    auto stack_alloc = kmalloc(allocated_size);
    if (!stack_alloc)
        return {};

    auto stack_ptr = static_cast<PtrData*>(stack_alloc.ptr);
    return Stack { KOwner<PtrData>(kernel_allocator(), *stack_ptr), allocated_size };
}

PtrData Stack::sp() const
{
    return reinterpret_cast<PtrData>(m_bottom.get()) + m_size;
}



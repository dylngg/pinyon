#pragma once

template <typename... Args>
inline void panic(Args&& ... args)
{
    asm volatile("b halt");
}


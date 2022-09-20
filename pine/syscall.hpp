#pragma once
#include <pine/types.hpp>

enum class Syscall : u32 {
    Yield = 0,
    Sleep,
    Read,
    Write,
    Sbrk,
    Uptime,
    CPUTime,
    Exit,
};

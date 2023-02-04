#pragma once
#include <pine/types.hpp>

enum class Syscall : u32 {
    Yield = 0,
    Sleep,
    Open,
    Read,
    Write,
    Close,
    Dup,
    Sbrk,
    Uptime,
    CPUTime,
    Exit,
};

enum class FileMode {
    Read,
    Write,
};

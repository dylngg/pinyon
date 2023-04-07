#pragma once
#include <pine/types.hpp>

enum class PrivilegeLevel {
    Userspace,
    Kernel,
};

enum class Syscall {
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
    ReadWrite,
};

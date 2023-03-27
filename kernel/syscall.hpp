#pragma once

#include <pine/types.hpp>
#include <pine/syscall.hpp>

PtrData handle_syscall(Syscall call, PtrData arg1, PtrData arg2, PtrData arg3);
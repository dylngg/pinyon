#pragma once

#include <pine/types.hpp>
#include <pine/syscall.hpp>

PtrData handle_syscall(PtrData call_data, PtrData arg1, PtrData arg2, PtrData arg3);
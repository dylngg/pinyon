#pragma once

#ifdef AARCH64
#include "arch/aarch64/barrier.hpp"
#elif AARCH32
#include "arch/aarch32/barrier.hpp"
#else
#error Architecture not defined
#endif

#pragma once

#ifdef AARCH64
#include "aarch64/barrier.hpp"
#elif AARCH32
#include "aarch32/barrier.hpp"
#else
#error Architecture not defined
#endif

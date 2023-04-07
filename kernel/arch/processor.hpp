#pragma once

#ifdef AARCH64
#include "aarch64/processor.hpp"
#elif AARCH32
#include "aarch32/processor.hpp"
#else
#error Architecture not defined
#endif

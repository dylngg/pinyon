#pragma once
#include <stddef.h>

[[nodiscard]] inline void* operator new(size_t, void* p) noexcept { return p; }

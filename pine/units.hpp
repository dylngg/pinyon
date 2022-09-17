#pragma once

#include <cstddef>
#include "types.hpp"

constexpr unsigned KiB = 1024;
constexpr unsigned MiB = 1048576;
constexpr unsigned GiB = 1073741824;
constexpr unsigned Alignment = alignof(max_align_t);

#define SYS_HZ 8
#define SYS_HZ_BITS 3

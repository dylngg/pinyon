#pragma once
#include "processor.hpp"

extern "C" {
void synchronous_kernel_handler(const Registers&);
}
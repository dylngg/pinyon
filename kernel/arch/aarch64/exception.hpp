#pragma once
#include "processor.hpp"

extern "C" {
void kernel_synchronous_handler(ESR_EL1);
}
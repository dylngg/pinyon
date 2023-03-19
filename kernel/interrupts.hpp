#pragma once

// These are implemented by the device/*/interrupts.cpp file

void interrupts_init();

void interrupts_enable_timer();

void interrupts_enable_uart();
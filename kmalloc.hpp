#pragma once
#include <pine/types.hpp>

void kmalloc_init();

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));
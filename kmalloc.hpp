#pragma once
#include <pine/types.hpp>

void kmalloc_init();

void kfree(void*);

void* kmalloc(size_t) __attribute__((malloc));

struct KMallocStats {
    size_t heap_size;
    size_t amount_used;
    u16 num_mallocs;
    u16 num_frees;
};

KMallocStats kmemstats();
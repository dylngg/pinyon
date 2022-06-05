#pragma once

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <vector>
#include <random>
#include <algorithm>

#include <pine/units.hpp>
#include <pine/units.hpp>
#include <pine/malloc.hpp>
#include <pine/alien/print.hpp>  // Need access to our print() ADL implementations (analogus to std::cout)

using namespace pine;

static size_t host_page_size = (size_t)sysconf(_SC_PAGESIZE);

static Pair<char*, size_t> allocate_scratch_page(unsigned num_pages = 1)
{
    // mmap a page so that we have true isolation
    errno = 0;
    void* scratch_page_ptr = mmap(nullptr, host_page_size * num_pages, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(errno == 0);

    memset(scratch_page_ptr, 0, num_pages * host_page_size);
    assert(host_page_size == PageSize);
    size_t scratch_page = reinterpret_cast<uintptr_t>(scratch_page_ptr) / host_page_size;
    return { static_cast<char*>(scratch_page_ptr), scratch_page };
}

void free_list_re_add()
{
    FreeList free_list;
    auto [ptr, _] = allocate_scratch_page();
    free_list.add(ptr, host_page_size);

    int i = 0;
    void* alloc_ptr = nullptr;
    do {
        auto allocation = free_list.try_reserve((size_t)16);
        alloc_ptr = allocation.ptr;
        ++i;

        if (alloc_ptr) {
            assert(is_pointer_aligned_two(alloc_ptr, Alignment));
            *((char*)alloc_ptr) = 1;  // try writing to it; checking if valid
            assert((char*)alloc_ptr < ((char*)ptr+host_page_size));
        }
    }
    while (alloc_ptr != nullptr);
    assert(i > 8);  // bare minimum

    auto allocation = free_list.try_reserve(0);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.try_reserve(1);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.try_reserve(16);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    auto [new_ptr, __] = allocate_scratch_page();
    free_list.add(new_ptr, host_page_size);

    i = 0;
    do {
        allocation = free_list.try_reserve(15);
        alloc_ptr = allocation.ptr;
        ++i;

        if (alloc_ptr) {
            assert(is_pointer_aligned_two(alloc_ptr, Alignment));
            *((char*)alloc_ptr) = 1;  // try writing to it; checking if valid
            assert((char*)alloc_ptr < ((char*)new_ptr+host_page_size));
        }
    }
    while (alloc_ptr != nullptr);
    assert(i > 8);  // bare minimum

    allocation = free_list.try_reserve(0);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.try_reserve(1);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.try_reserve(16);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);
}

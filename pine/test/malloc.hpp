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
#include <pine/limits.hpp>
#include <pine/malloc.hpp>
#include <pine/print.hpp>
#include <pine/alien/print.hpp>  // Need access to our print() ADL implementations (analogus to std::cout)

using namespace pine;

static size_t host_page_size = (size_t)sysconf(_SC_PAGESIZE);

static Pair<char*, size_t> allocate_scratch_page(unsigned num_pages = 1)
{
    size_t total_size = host_page_size * num_pages;
    // mmap a page so that we have true isolation
    errno = 0;
    void* scratch_page_ptr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(errno == 0);
    assert(scratch_page_ptr != nullptr);

    memset(scratch_page_ptr, 0, total_size);
    return { static_cast<char*>(scratch_page_ptr), total_size };
}

static void free_scratch_page(void* ptr, size_t size)
{
    assert(munmap(ptr, size) == 0);
}

void free_list_re_add()
{
    IntrusiveFreeList free_list;
    auto [ptr, _] = allocate_scratch_page();
    free_list.add(ptr, host_page_size);

    int i = 0;
    void* alloc_ptr = nullptr;
    do {
        auto allocation = free_list.allocate((size_t)16);
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

    auto allocation = free_list.allocate(0);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.allocate(1);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.allocate(16);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    auto [new_ptr, __] = allocate_scratch_page();
    free_list.add(new_ptr, host_page_size);

    i = 0;
    do {
        allocation = free_list.allocate(15);
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

    allocation = free_list.allocate(0);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.allocate(1);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);

    allocation = free_list.allocate(16);
    alloc_ptr = allocation.ptr;
    assert(alloc_ptr == nullptr);
}

void page_allocator_backend_create()
{
    // Try allocate differing sizes; typically 12-20 for 32-bit machine
    unsigned max_page_bit_width = 31;
    for (unsigned i = 0; i < max_page_bit_width; i++) {
        auto [ptr, size] = allocate_scratch_page(4);
        {  // Make sure allocator is destructed before we free the scratch page!
            PageAllocatorBackend allocator;
            allocator.init(PageRegion { 0, 1u << max_page_bit_width },  PageRegion::from_ptr(ptr, size));

            unsigned num_pages = 1 << i;
            auto region = allocator.allocate(num_pages);
            assert(region.size != 0);
            assert(region.size == num_pages * PageSize);
        }
        free_scratch_page(ptr, size);
    }

    // Try allocating successfully then allocating again (failing)
    auto ptr_and_size = allocate_scratch_page(4);
    auto ptr = ptr_and_size.first;
    auto size = ptr_and_size.second;
    {  // Make sure allocator is destructed before we free the scratch page!
        PageAllocatorBackend allocator;
        allocator.init(PageRegion { 0, 1 },  PageRegion::from_ptr(ptr, size));

        auto allocation = allocator.allocate(1);
        assert(allocation.ptr == nullptr);
        assert(allocation.size != 0);
        assert(allocation.size == PageSize);

        for (unsigned i = 0; i < max_page_bit_width; i++) {
            unsigned num_pages = 1 << i;
            allocation = allocator.allocate(num_pages);
            assert(allocation.size == 0);
        }
    }
    free_scratch_page(ptr, size);

    // Try allocating sizes larger than given
    ptr_and_size = allocate_scratch_page(4);
    ptr = ptr_and_size.first;
    size = ptr_and_size.second;
    {  // Make sure allocator is destructed before we free the scratch page!
        PageAllocatorBackend allocator;
        allocator.init(PageRegion { 0, 1 },  PageRegion::from_ptr(ptr, size));

        for (unsigned i = 1; i < max_page_bit_width; i++) {
            unsigned num_pages = 1 << i;
            auto allocation = allocator.allocate(num_pages);
            assert(allocation.size == 0);
        }
    }
    free_scratch_page(ptr, size);
}

#include "kmalloc.hpp"
#include <pine/console.hpp>
#include <pine/types.hpp>

// Somewhat arbitrary, but if we're allocating more than 64MiB I'm concerned...
#define MAX_SIZE 67108864
#define MIN_SIZE 8
#define NEW_BLOCK_SIZE 4096

/*
 * If we get virtual memory working I estimate we'll keep
 * our page tables, stack, etc under this mark...
 *
 * Devices start at 0x3F000000, so avoid going past that.
 */
#define HEAP_START 0x00440000
#define HEAP_END 0x3EFFF000

struct Header {
    Header(Header* prev_header, Header* next_header, size_t block_size, bool is_free)
        : prev(prev_header)
        , next(next_header)
        , size(block_size)
        , free(is_free)
    {
    }

    constexpr bool is_free(size_t requested_size) const
    {
        return free && size >= requested_size;
    }
    constexpr void* user_ptr()
    {
        return this + 1;
    }
    constexpr Header* next_header() const
    {
        return next;
    }
    Header* calc_next_header()
    {
        return (Header*)((unsigned char*)(this) + sizeof(*this) + size);
    }

    Header* create_next_header(size_t requested_size)
    {
        auto* new_next_header = calc_next_header();
        *new_next_header = { this, next, requested_size, true };

        if (next) {
            // Ensure next points back to this new split, rather than ourselves
            next->prev = new_next_header;
        }

        next = new_next_header;
        return new_next_header;
    }

    void reserve(size_t requested_size)
    {
        size_t min_block_size = sizeof(*this) + MIN_SIZE;

        if (size - requested_size > min_block_size) {
            // can split into two blocks
            size_t remaining_size = size - requested_size - sizeof(*this);
            size = requested_size;
            free = false;

            create_next_header(remaining_size);
            return;
        }

        // no need for splitting, or really anything at all... :)
        free = false;
    }

    void destroy()
    {
        free = true;

        if (next && next->free) {
            // Coalesce right block into ours
            size += sizeof(*this) + next->size;
            next = next->next; // now our next is our adopted's next

            if (next && next->prev) {
                // ensure next's prev header (previously next->next) points to
                // us, rather than the coalesced block we've adopted
                next->prev = this;
            }
        }
        if (prev && prev->free) {
            // Coalese left block; take recursive approach and let prev adopt us
            prev->destroy();
            return;
        }
        return;
    }

private:
    Header* prev;
    Header* next;
    u32 size;
    bool free;
};

#define NEW_SIZE ((NEW_BLOCK_SIZE) - (sizeof(Header)))

static Header* first_free_header = (Header*)HEAP_START;

void kmalloc_init()
{
    *first_free_header = Header { nullptr, nullptr, NEW_SIZE, true };
}

void* kmalloc(size_t requested_size)
{
    if (requested_size > MAX_SIZE) {
        consolef("kmalloc:\trequested_size requested is too large!");
        return nullptr;
    }

    auto* curr_header = first_free_header;
    while ((PtrData)curr_header < HEAP_END) {
        if (curr_header->is_free(requested_size)) {
            curr_header->reserve(requested_size);
            return curr_header->user_ptr();
        }

        auto* next_header = curr_header->next_header();
        if (!next_header) {
            // Allocate more memory, setup new header
            auto* new_header = curr_header->create_next_header(NEW_SIZE);
            new_header->reserve(requested_size);
            return new_header->user_ptr();
        }
        curr_header = next_header;
    }

    // No free space?!
    consolef("kmalloc:\tNo free space available?!");
    return nullptr;
}

void kfree(void* ptr)
{
    auto* freed_header = (Header*)((unsigned char*)ptr - sizeof(Header));
    freed_header->destroy();
}
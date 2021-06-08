#pragma once
#include <pine/barrier.hpp>
#include <pine/types.hpp>

/*
 * This is an absolutely terrible implementation of malloc. It is extremely
 * slow due to each memory allocation requiring a walk of the _entire_ memory
 * allocation list... shield your eyes if they are sensitive to that kind of
 * thing.
 *
 * Implemented in a header because we use template to allow for a kernel and
 * "userspace" malloc implementation. The alternative to this is virtual
 * classes, but those don't play well in our embedded environment.
 */

#define MIN_SIZE 8
#define NEW_BLOCK_SIZE 4096

struct MallocStats {
    size_t heap_size;
    size_t amount_used;
    u16 num_mallocs;
    u16 num_frees;
};

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
    constexpr size_t user_size()
    {
        return size;
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

    size_t destroy()
    {
        free = true;

        if (next && next->free) {
            // Coalesce right block into ours
            size += sizeof(*next) + next->size;
            next = next->next; // now our next is our adopted's next

            if (next && next->prev) {
                // ensure next's prev header (previously next->next) points to
                // us, rather than the coalesced block we've adopted
                next->prev = this;
            }
        }
        if (prev && prev->free) {
            // Coalese left block; take recursive approach and let prev adopt us
            return prev->destroy();
        }
        return size;
    }

private:
    Header* prev;
    Header* next;
    u32 size;
    bool free;
};

#define NEW_SIZE ((NEW_BLOCK_SIZE) - (sizeof(Header)))

template <class MemoryBounds>
class MemoryAllocator {
public:
    MemoryAllocator(MemoryBounds* mem_bounds)
        : m_mem_bounds(mem_bounds)
    {
        auto maybe_heap_size = m_mem_bounds->try_extend_heap(NEW_BLOCK_SIZE * 8);
        // FIXME: Handle this better
        size_t heap_size = 0;
        if (maybe_heap_size)
            heap_size = maybe_heap_size.value();

        m_stats = { .heap_size = heap_size, .amount_used = 0, .num_mallocs = 0, .num_frees = 0 };
        m_first_free_header = (Header*)m_mem_bounds->heap_start();
        *m_first_free_header = Header { nullptr, nullptr, NEW_SIZE, true };
    }

    void* allocate(size_t requested_size)
    {
        m_stats.num_mallocs++;
        auto* curr_header = m_first_free_header;
        while (curr_header) {
            if (curr_header->is_free(requested_size)) {
                curr_header->reserve(requested_size);
                m_stats.amount_used += requested_size;
                return curr_header->user_ptr();
            }

            auto* next_header = curr_header->next_header();
            if (!next_header) {
                size_t extend_size = requested_size > NEW_SIZE ? requested_size : NEW_SIZE;
                auto maybe_heap_incr_size = m_mem_bounds->try_extend_heap(extend_size);
                if (!maybe_heap_incr_size)
                    // No free space?!
                    return nullptr;

                size_t heap_incr_size = maybe_heap_incr_size.value();

                m_stats.heap_size += heap_incr_size;

                auto* new_header = curr_header->create_next_header(extend_size);
                new_header->reserve(requested_size);
                m_stats.amount_used += requested_size;
                return new_header->user_ptr();
            }
            curr_header = next_header;
        }

        // No free space?!
        return nullptr;
    }

    void free(void* ptr)
    {
        m_stats.num_frees++;
        auto* freed_header = (Header*)((unsigned char*)ptr - sizeof(Header));
        size_t size_freed = freed_header->user_size();
        m_stats.amount_used -= size_freed;
        freed_header->destroy();
    }

    MallocStats stats() const { return m_stats; };

private:
    MemoryBounds* m_mem_bounds;
    MallocStats m_stats;
    Header* m_first_free_header {};
};
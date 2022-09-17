#pragma once
#include "units.hpp"
#include "print.hpp"

constexpr unsigned PageSize = 4u * KiB;
constexpr unsigned HugePageSize = 1u * MiB;

struct Region {
    unsigned offset;
    unsigned length;

    [[nodiscard]] static Region from_ptr(void* ptr, size_t size_in_bytes);
    [[nodiscard]] void* ptr() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool fits(unsigned other_length) const;
    [[nodiscard]] bool aligned_to(unsigned alignment) const;
    [[nodiscard]] unsigned end_offset() const;
    [[nodiscard]] void* end_ptr() const;
    [[nodiscard]] Pair<Region, Region> halve() const;
    [[nodiscard]] Pair<Region, Region> split_left(unsigned num_pages) const;

    explicit operator bool() const { return length != 0; };
    bool operator==(const Region& other) const;
    bool operator>(const Region& other) const;
    bool operator<(const Region& other) const;

    friend void print_with(pine::Printer&, const Region& region);
};

struct SectionRegion : public Region {
    [[nodiscard]] static Region from_range(PtrData start, PtrData end);
};
using PageRegion = Region;

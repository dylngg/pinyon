#pragma once
#include "units.hpp"
#include "print.hpp"

constexpr unsigned PageSize = 4u * KiB;
constexpr unsigned HugePageSize = 1u * MiB;

template <unsigned Magnitude>
struct Region {
    unsigned offset;
    unsigned length;

    [[nodiscard]] static Region from_ptr(void* ptr, size_t size_in_bytes)
    {
        return {
            static_cast<unsigned int>(reinterpret_cast<PtrData>(ptr) / Magnitude),
            static_cast<unsigned int>(size_in_bytes / Magnitude)
        };
    }
    [[nodiscard]] void* ptr() const
    {
        return reinterpret_cast<void*>(static_cast<PtrData>(offset) * Magnitude);
    }
    [[nodiscard]] size_t size() const
    {
        return static_cast<size_t>(length) * Magnitude;
    }
    [[nodiscard]] bool fits(unsigned other_length) const
    {
        return length >= other_length;
    }
    [[nodiscard]] bool aligned_to(unsigned alignment) const
    {
        return offset % alignment == 0;
    }
    [[nodiscard]] unsigned end_offset() const
    {
        return offset + length;
    }
    [[nodiscard]] void* end_ptr() const
    {
        return reinterpret_cast<void*>(static_cast<PtrData>(end_offset()) * Magnitude);
    }
    [[nodiscard]] Pair<Region<Magnitude>, Region<Magnitude>> halve() const
    {
        auto halved_length = length / 2;
        return {
            { offset, halved_length },
            { offset + halved_length, halved_length }
        };
    }
    [[nodiscard]] Pair<Region, Region> split_left(unsigned num_pages) const
    {
        return {
            { offset, num_pages },
            { offset + num_pages, length - num_pages }
        };
    }

    explicit operator bool() const { return length != 0; };
    bool operator==(const Region& other) const
    {
        return offset == other.offset && length == other.length;
    }
    bool operator>(const Region& other) const
    {
        return (
            offset > other.offset
            || (offset == other.offset && length > other.length)
        );
    }
    bool operator<(const Region& other) const
    {
        return (
            offset < other.offset
            || (offset == other.offset && length < other.length)
        );
    }

    static Region<Magnitude> from_range(PtrData start, PtrData end)
    {
        return { start / Magnitude, (end - start) / Magnitude };
    }
    template <unsigned NewMagnitude>
    static Region<NewMagnitude> convert_to(Region<Magnitude> region)
    {
        static_assert(Magnitude % NewMagnitude == 0);
        static_assert(Magnitude >= NewMagnitude);
        constexpr unsigned factor = Magnitude / NewMagnitude;
        return { region.offset * factor, region.length * factor };
    }

    friend void print_with(pine::Printer& printer, const Region& region)
    {
        print_each_with(printer, "(");
        print_each_with(printer, region.offset, ",", region.length);
        print_with(printer, ")");
    }
};

using PageRegion = Region<PageSize>;
using SectionRegion = Region<HugePageSize>;

inline PageRegion as_page_region(SectionRegion section_region)
{
    return SectionRegion::convert_to<PageSize>(section_region);
}
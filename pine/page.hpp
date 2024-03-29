#pragma once
#include "units.hpp"
#include "print.hpp"

constexpr size_t PageSize = 4u * KiB;
constexpr size_t SectionSize = 1u * MiB;

template <size_t Magnitude>
struct Region {
    size_t offset;
    size_t length;

    [[nodiscard]] static Region from_ptr(void* ptr, size_t size_in_bytes)
    {
        return {
            .offset = reinterpret_cast<PtrData>(ptr) / Magnitude,
            .length = pine::divide_up(size_in_bytes, Magnitude)
        };
    }
    [[nodiscard]] void* ptr(size_t left_offset = 0) const
    {
        return reinterpret_cast<void*>((static_cast<PtrData>(offset) + left_offset) * Magnitude);
    }
    [[nodiscard]] size_t size() const
    {
        return static_cast<size_t>(length) * Magnitude;
    }
    [[nodiscard]] bool fits(size_t other_length) const
    {
        return length >= other_length;
    }
    [[nodiscard]] bool contains(const Region& other) const
    {
        return offset <= other.offset && end_offset() >= other.end_offset();
    }
    [[nodiscard]] bool aligned_to(size_t alignment) const
    {
        return offset % alignment == 0;
    }
    [[nodiscard]] size_t end_offset() const
    {
        return offset + length;
    }
    [[nodiscard]] void* end_ptr() const
    {
        return reinterpret_cast<void*>(static_cast<PtrData>(end_offset()) * Magnitude);
    }
    [[nodiscard]] Pair<Region<Magnitude>, Region<Magnitude>> halve() const
    {
        auto remainder = length % 2;
        auto halved_length = length / 2;
        return {
            { offset, halved_length },
            { offset + halved_length, halved_length + remainder }
        };
    }
    [[nodiscard]] Pair<Region, Region> split_left(size_t num_pages) const
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
    bool operator>=(const Region& other) const
    {
        return (
            offset >= other.offset
            || (offset == other.offset && length >= other.length)
        );
    }
    bool operator<(const Region& other) const
    {
        return (
            offset < other.offset
            || (offset == other.offset && length < other.length)
        );
    }
    bool operator<=(const Region& other) const
    {
        return (
            offset <= other.offset
            || (offset == other.offset && length <= other.length)
        );
    }

    static Region<Magnitude> from_range(PtrData start, PtrData end)
    {
        return { start / Magnitude, (end - start) / Magnitude };
    }
    template <size_t NewMagnitude>
    static Region<NewMagnitude> convert_to(Region<Magnitude> region)
    {
        static_assert(Magnitude % NewMagnitude == 0);
        static_assert(Magnitude >= NewMagnitude);
        constexpr size_t factor = Magnitude / NewMagnitude;
        return { region.offset * factor, region.length * factor };
    }

    friend void print_with(pine::Printer& printer, const Region& region)
    {
        print_each_with_spacing(printer, "(");
        print_each_with_spacing(printer, region.offset, ",", region.length);
        print_with(printer, ")");
    }
};

using PageRegion = Region<PageSize>;
using SectionRegion = Region<SectionSize>;

inline PageRegion as_page_region(SectionRegion section_region)
{
    return SectionRegion::convert_to<PageSize>(section_region);
}

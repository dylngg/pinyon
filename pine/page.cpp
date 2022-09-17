#include "page.hpp"
#include "limits.hpp"
#include "print.hpp"

Region Region::from_ptr(void* ptr, size_t size_in_bytes)
{
    return {
        static_cast<unsigned int>(reinterpret_cast<PtrData>(ptr) / PageSize),
        static_cast<unsigned int>(size_in_bytes / PageSize)
    };
}

void* Region::ptr() const
{
    return reinterpret_cast<void*>(static_cast<PtrData>(offset) * PageSize);
}

size_t Region::size() const
{
    return static_cast<size_t>(length) * PageSize;
}

bool Region::fits(unsigned other_length) const
{
    return length >= other_length;
}

bool Region::aligned_to(unsigned alignment) const
{
    return offset % alignment == 0;
}

unsigned Region::end_offset() const
{
    return offset + length;
}

void* Region::end_ptr() const
{
    return reinterpret_cast<void*>(static_cast<PtrData>(end_offset()) * PageSize);
}

Pair<Region, Region> Region::halve() const
{
    auto halved_length = length / 2;
    return {
        Region {offset, halved_length },
        Region {offset + halved_length, halved_length }
    };
}

Pair<Region, Region> Region::split_left(unsigned num_pages) const
{
    return {
        Region { offset, num_pages },
        Region { offset + num_pages, length - num_pages }
    };
}

bool Region::operator==(const Region& other) const
{
    return offset == other.offset && length == other.length;
}

bool Region::operator>(const Region& other) const
{
    return (
        offset > other.offset
        || (offset == other.offset && length > other.length)
    );
}

bool Region::operator<(const Region& other) const
{
    return (
        offset < other.offset
        || (offset == other.offset && length < other.length)
    );
}

void print_with(pine::Printer& printer, const Region& region)
{
    print_with(printer, "Region(");
    print_each_with(printer, region.offset, ",", region.length);
    print_with(printer, ")");
}

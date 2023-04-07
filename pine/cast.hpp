#pragma once
#include <pine/types.hpp>
#include <pine/limits.hpp>

template <typename Into>
constexpr bool fits_within(PtrData from)
{
    if constexpr (pine::is_signed<Into>) {
        if (pine::bit_cast<ptrdiff_t>(from) > static_cast<ptrdiff_t>(pine::limits<Into>::max))
            return false;
        if (pine::bit_cast<ptrdiff_t>(from) < static_cast<ptrdiff_t>(pine::limits<Into>::min))
            return false;
        return true;
    }
    if (from > static_cast<PtrData>(pine::limits<Into>::max))
        return false;
    if (from < static_cast<PtrData>(pine::limits<Into>::min))
        return false;
    return true;
}

template <typename To, typename From>
constexpr To to_signed_cast(From from)
{
    static_assert(pine::is_signed<To>);
    return static_cast<To>(pine::bit_cast<ptrdiff_t>(from));
}

template <typename To, typename From>
constexpr To from_signed_cast(From from)
{
    static_assert(pine::is_signed<From>);
    return pine::bit_cast<To>(static_cast<ptrdiff_t>(from));
}

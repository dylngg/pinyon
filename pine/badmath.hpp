#pragma once

unsigned long absl(long num);

unsigned int powui(unsigned int num, unsigned int power);

unsigned int log10ul(unsigned long num);

template <typename SizeType>
constexpr const SizeType& max(const SizeType& first, const SizeType& second)
{
    return (first > second) ? first : second;
}

template <typename SizeType>
constexpr const SizeType& min(const SizeType& first, const SizeType& second)
{
    return (first < second) ? first : second;
}

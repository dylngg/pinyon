#pragma once

unsigned long absl(long num);

unsigned int powui(unsigned int num, unsigned int power);

unsigned long log10ul(unsigned long num);

template <typename SizeType>
SizeType max(SizeType first, SizeType second)
{
    return (first > second) ? first : second;
}

template <typename SizeType>
SizeType min(SizeType first, SizeType second)
{
    return (first < second) ? first : second;
}
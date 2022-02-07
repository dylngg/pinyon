/*
 * Note: The following is patchwork of functions that were all implemented
 * quickly to solve an immediate need. They all pretty much suffer from
 * performance and correctness problems, partly stemming from the author's
 * ignorance of math.
 */
#include "badmath.hpp"

unsigned long absl(long num)
{
    return num >= 0 ? num : -num;
}

unsigned int powui(unsigned int num, unsigned int power)
{
    if (power == 0)
        return 1;

    unsigned int result = num;
    while (power > 1) {
        result *= num;
        power--;
    }
    return result;
}

unsigned int log10ul(unsigned long num)
{
    unsigned int log = 0;
    while (num >= 10) {
        // log10(x) = y if b^y = x
        num /= 10;
        log++;
    }
    return log;
}

#include "string.hpp"
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include "stdarg.h"
#include <pine/badmath.hpp>
#include <pine/types.hpp>

void zero_out(void* target, size_t size)
{
    unsigned char* _target = (unsigned char*)target;
    while (size-- > 0)
        *_target = 0;
}

void strcpy(char* __restrict__ to, const char* from)
{
    while (*from != '\0') {
        *to = *from;
        to++;
        from++;
    }
    *to = '\0';
}

size_t strlen(const char* string)
{
    size_t offset = 0;
    while (string[offset] != '\0') {
        offset++;
    }
    return offset;
}

int strcmp(const char* first, const char* second)
{
    size_t offset = 0;
    while (first[offset] != '\0' && second[offset] != '\0') {
        int diff = (unsigned char)first[offset] - (unsigned char)second[offset];
        if (diff > 0)
            return 1;
        if (diff < 0)
            return -1;
        offset++;
    }
    return first[offset] != second[offset];
}

void ltoa10(char* buf, long num)
{
    // FIXME: This function is not particularly efficient, partly thanks
    //        to the following log madness and the overzealous %, pow and / below
    unsigned long absnum = absl(num);
    if (num < 0) {
        buf[0] = '-';
        buf++;
    }

    ultoa10(buf, absnum);
}

void ultoa10(char* buf, unsigned long num)
{
    // FIXME: This function is not particularly efficient, partly thanks
    //        to the following log madness and the overzealous %, pow and / below
    unsigned long len = log10ul(num) + 1;
    unsigned int pos = 0;

    for (; pos < len; pos++) {
        // Basically we strip the leading digits till pos and then divide by
        // the correct base to get the number.
        // e.g. want 3 of 345 -> 345 % 10^3 (=345) / 10^2 (=3.45)
        //           4 of 345 -> 345 % 10^2 (=45)  / 10^1 (=4.5)
        //           5 of 345 -> 345 % 10^1 (=5)   / 10^0 (=5.0)
        unsigned int digit = (num % powui(10, len - pos)) / powui(10, len - pos - 1);
        buf[pos] = '0' + digit;
    }
    buf[pos] = '\0';
}

void ultoa16(char* buf, unsigned long num, ToAFlag flag)
{
    buf[0] = '0';
    buf[1] = 'x';
    buf += 2;

    int pos = 0;
    for (; pos < 8; pos++)
        buf[pos] = '0';

    buf[8] = '\0';

    pos = 7;
    while (pos >= 0 && num > 0) {
        unsigned long hex_num = num % 16;
        if (hex_num < 10)
            buf[pos] = hex_num + '0';
        else
            buf[pos] = hex_num + ((flag == ToAUpper ? 'A' : 'a') - 10);

        num >>= 4; // divide by 16, but no need for division
        pos--;
    }
}

size_t strbufcat(char* buf, const StringView& str, size_t start, size_t buf_size)
{
    if (start >= buf_size)
        return 0;

    size_t offset = 0;
    for (auto ch : str) {
        if (start + offset + 1 >= buf_size)
            break;

        buf[start + offset] = ch;
        offset++;
    }
    buf[start + offset] = '\0';
    return offset;
}
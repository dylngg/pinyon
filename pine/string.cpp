#include <pine/string.hpp>
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include "stdarg.h"
#include <pine/badmath.hpp>
#include <pine/types.hpp>

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
        int diff = first[offset] - second[offset];
        if (diff > 0)
            return 1;
        if (diff < 0)
            return -1;
        offset++;
    }
    return first[offset] != second[offset];
}

void itoa10(char* buf, int num)
{
    // FIXME: This function is not particularly efficient, partly thanks
    //        to the following log madness and the overzealous %, pow and / below
    unsigned int absnum = absui(num);
    unsigned int len = log10ui(absnum) + 1;
    unsigned int pos = 0;
    if (num < 0) {
        buf[0] = '-';
        buf++;
    }

    for (; pos < len; pos++) {
        // Basically we strip the leading digits till pos and then divide by
        // the correct base to get the number.
        // e.g. want 3 of 345 -> 345 % 10^3 (=345) / 10^2 (=3.45)
        //           4 of 345 -> 345 % 10^2 (=45)  / 10^1 (=4.5)
        //           5 of 345 -> 345 % 10^1 (=5)   / 10^0 (=5.0)
        unsigned int digit = (absnum % powui(10, len - pos)) / powui(10, len - pos - 1);
        buf[pos] = '0' + digit;
    }
    buf[pos] = '\0';
}

size_t strbufcat(char* buf, const String& str, size_t start, size_t buf_size)
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

size_t vstrbufprintf(char* buf, size_t buf_size, const String& fmt, va_list args)
{
    if (buf == nullptr)
        return 0;

    buf[0] = '\0';
    size_t buf_pos = 0;

    const auto& add_string_to_buf = [&](const String& str) -> bool {
        buf_pos += strbufcat(buf, str, buf_pos, buf_size);
        return buf_pos + 1 < buf_size;
    };
    vfnprintf(add_string_to_buf, fmt, args);
    return buf_pos + 1;
}

size_t strbufprintf(char* buf, size_t buf_size, const String& fmt, ...)
{
    va_list args;
    // FIXME: Clang-tidy is freaking out that va_start with a reference has undefined behavior...
    va_start(args, fmt);
    return vstrbufprintf(buf, buf_size, fmt, args);
}
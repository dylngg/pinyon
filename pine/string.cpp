#include "string.hpp"
#include <pine/printf.hpp>
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include <cstdarg>
#include <pine/badmath.hpp>
#include <pine/types.hpp>

size_t strcopy(char* __restrict__ to, const char* from)
{
    size_t copied = 0;
    while (*from != '\0') {
        *to = *from;
        to++;
        from++;
        copied++;
    }
    *to = '\0';
    return copied;
}

size_t strbufcopy(char* __restrict__ buf, size_t bufsize, const char* from)
{
    if (bufsize == 0)
        return 0;

    size_t copied = 0;
    while (*from != '\0' && copied + 1 < bufsize) { // + '\0'
        *buf = *from;
        buf++;
        from++;
        copied++;
    }
    *buf = '\0';
    return copied;
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
            buf[pos] = hex_num + ((flag == ToAFlag::Upper ? 'A' : 'a') - 10);

        num >>= 4; // divide by 16, but no need for division
        pos--;
    }
}

size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vsbufprintf(buf, bufsize, fmt, args);
}

size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args)
{
    size_t buf_pos = 0;
    if (bufsize == 0)
        return 0;

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        size_t message_size = strlen(message);
        size_t copied = strbufcopy(buf + buf_pos, bufsize - buf_pos, message);
        buf_pos += copied;
        return copied == message_size;
    };

    vfnprintf(try_add_wrapper, fmt, args);
    return buf_pos;
}

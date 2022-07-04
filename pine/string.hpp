#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include <cstdarg>
#include "iter.hpp"
#include "limits.hpp"
#include "metaprogramming.hpp"
#include "types.hpp"

namespace pine {

size_t strcopy(char* to, const char* from);
size_t strbufcopy(char* __restrict__ buf, size_t bufsize, const char* from);
size_t strlen(const char* string);
int strcmp(const char* first, const char* second);
size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args);

enum class ToAFlag : int {
    Upper,
    Lower,
};

template <typename Int, enable_if<is_integer<Int>, Int>* = nullptr>
void to_strbuf(char* buf, size_t bufsize, Int num)
{
    if (bufsize < limits<Int>::digits + 1)
        return;

    Int absnum = num;
    if constexpr (is_signed<Int>) {
        if (num < 0) {
            absnum = abs(num);
            buf[0] = '-';
            ++buf;
            --bufsize;
        }
    }

    Int len = log(absnum) + 1;
    for (Int pos = 0; pos < len && static_cast<size_t>(pos) < bufsize; pos++) {
        // e.g. 345
        Int divisor = pow(static_cast<Int>(10), len - pos - 1);
        Int digit = absnum / divisor;  // 345 / 100 = 3
        buf[pos] = static_cast<char>('0' + digit);
        absnum -= digit * divisor;  // 345 - 300 = 45
    }

    buf[len] = '\0';
}

template <typename UInt, enable_if<is_unsigned<UInt>, UInt>* = nullptr>
void to_strbuf_hex(char* buf, size_t bufsize, UInt num, ToAFlag flag)
{
    if (bufsize < limits<UInt>::digits + 1)
        return;

    buf[0] = '0';
    buf[1] = 'x';
    buf += 2;

    int pos = 0;
    for (; pos < 8; pos++)
        buf[pos] = '0';

    buf[8] = '\0';

    pos = 7;
    while (pos >= 0 && num > 0) {
        int hex_num = num % 16;
        if (hex_num < 10)
            buf[pos] = static_cast<char>(hex_num + '0');
        else
            buf[pos] = static_cast<char>((hex_num - 10) + (flag == ToAFlag::Upper ? 'A' : 'a'));

        num >>= 4; // divide by 16, but no need for division
        pos--;
    }
}

struct StringView {
public:
    StringView(const char* string)
        : m_chars(string)
        , m_length(strlen(string))
    {
    }

    bool operator==(const StringView& other) const;
    const char& operator[](size_t pos) const;

    using ConstIter = RandomAccessIter<const StringView, const char>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    size_t length() const { return m_length; }
    const char* data() const { return m_chars; }

private:
    const char* m_chars;
    const size_t m_length;
};

}

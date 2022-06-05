#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include <cstdarg>
#include <pine/iter.hpp>
#include <pine/types.hpp>

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

void itoa10(char* buf, int num);
void uitoa10(char* buf, unsigned int num);
void ltoa10(char* buf, long num);
void ultoa10(char* buf, unsigned long num);
void ultoa16(char* buf, unsigned long num, ToAFlag flag);

struct StringView {
public:
    StringView(const char* string)
        : m_chars(string)
        , m_length(strlen(string))
    {
    }

    bool operator==(const StringView& other) const;
    const char& operator[](size_t pos) const;

    using ConstIter = SeqIter<const StringView, const char>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    size_t length() const { return m_length; }

private:
    const char* m_chars;
    const size_t m_length;
};

}

#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include <cstdarg>
#include <pine/iter.hpp>
#include <pine/types.hpp>

void zero_out(void* target, size_t size);
void memcopy(void* __restrict__ to, const void* __restrict__ from, size_t size);
size_t strcopy(char* __restrict__ to, const char* from);
size_t strbufcopy(char* __restrict__ buf, size_t bufsize, const char* from);
size_t strlen(const char* string);
int strcmp(const char* first, const char* second);
size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args);

enum class ToAFlag : int {
    Upper,
    Lower,
};

void ltoa10(char* buf, long num);
void ultoa10(char* buf, unsigned long num);
void ultoa16(char* buf, unsigned long num, ToAFlag flag);

struct StringView {
public:
    StringView(const char* string)
        : __chars(string)
        , __length(strlen(string))
    {
    }

    bool operator==(const StringView& other) const
    {
        if (__length != other.__length)
            return false;
        return strcmp(__chars, other.__chars) == 0;
    }
    const char& operator[](size_t pos) const
    {
        static char dummy = '\0';
        // FIXME: I'd rather have an assert here than this wackyness; this will just
        //        hide bugs
        if (pos >= __length)
            return dummy;
        return __chars[pos];
    }

    using ConstIter = FwdIter<const StringView, const char>;
    ConstIter begin() const { return ConstIter::begin(*this); }
    ConstIter end() const { return ConstIter::end(*this); }

    size_t length() const { return __length; }

private:
    const char* __chars;
    const size_t __length;
};
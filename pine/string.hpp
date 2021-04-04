#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include "stdarg.h"
#include <pine/iter.hpp>
#include <pine/types.hpp>

size_t strlen(const char* string);
int strcmp(const char* first, const char* second);
void itoa10(char* buf, int num);
void ultoa16(char* buf, unsigned long num);

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
        // FIXME: I'd rather have an assert here than this wackyness; this will just
        //        hide bugs
        if (pos >= __length)
            return __chars[__length - 1];
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
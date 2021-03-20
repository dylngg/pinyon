#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include "stdarg.h"
#include <pine/badmath.hpp>
#include <pine/string.hpp>

template <typename TryAddStringFunc>
size_t vfnprintf(TryAddStringFunc& try_add_string, const String& fmt, va_list args)
{
    // Keep a fake two char string on the stack for easy adding of single or dual chars
    char chstr[2] = " ";

    bool expect_fmt_ch = false;
    for (auto iter = fmt.begin(); !iter.at_end(); iter++) {
        auto ch = *iter;
        chstr[0] = ch;

        if (!expect_fmt_ch) {
            if (ch == '%') {
                expect_fmt_ch = true;
                continue;
            }
            if (!try_add_string(chstr))
                return false;

            continue;
        }

        // Format specifier: %s
        switch (ch) {
        case 's': {
            const char* str = va_arg(args, const char*);
            if (!try_add_string(str))
                return false;

            break;
        }
        case '%':
            if (!try_add_string(chstr))
                return false;

            break;
        case 'c': {
            int arg_ch = va_arg(args, int);
            chstr[0] = arg_ch;
            if (!try_add_string(chstr))
                return false;

            break;
        }
        case 'd': {
            int num = va_arg(args, int);
            char digits[12];
            itoa10(digits, num);
            if (!try_add_string(digits))
                return false;

            break;
        }
        default: {
            // FIXME: We would assert here if we could, but we do not have
            //        that so just print the incorrect fmt ch with a %
            int fmt_ch = va_arg(args, int); // va_arg upcasts to int...
            char fmt_strch[3] = { '%', (char)fmt_ch, '\0' };
            if (!try_add_string(fmt_strch))
                return false;

            break;
        }
        }
        expect_fmt_ch = false;
    }
    return true;
}

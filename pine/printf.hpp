#pragma once
// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include "stdarg.h"
#include <pine/badmath.hpp>
#include <pine/string.hpp>

enum ArgModifiers {
    ModNone,
    ModPointer,
    ModLong,
};

template <typename TryAddStringFunc>
size_t vfnprintf(TryAddStringFunc& try_add_string, const char* fmt, va_list args)
{
    ArgModifiers mod;
    // Keep a fake two char string on the stack for easy adding of single or dual chars
    char chstr[2] = " ";

    auto fmt_view = StringView(fmt);
    for (auto iter = fmt_view.begin(); !iter.at_end(); iter++) {
        auto ch = *iter;
        chstr[0] = ch;

        if (ch != '%') {
            // Literal
            try_add_string(chstr);
            continue;
        }

        // Format specifier: %s

        iter++; // consume '%'
        mod = ModNone;

        char type_or_mod_ch = *iter;
        if (type_or_mod_ch == 'l') {
            mod = ModLong;
            iter++; // consume 'l'
        }

        char type_ch = *iter;
        switch (type_ch) {
        case 's':
            if (!print_string(args, try_add_string))
                return false;
            break;

        case '%':
            if (!try_add_string(chstr))
                return false;
            break;

        case 'c':
            if (!print_char(args, try_add_string))
                return false;
            break;

        case 'u':
            [[fallthrough]];
        case 'd':
            if (!print_int(type_ch, args, try_add_string, mod))
                return false;
            break;

        case 'p':
            if (!print_pointer(args, try_add_string))
                return false;
            break;

        default:
            // Bail here. The caller may have provided an argument and if we
            // proceed then future arguments may be misaligned...
            // FIXME: Perhaps there is a better way to deal with this...
            return false;
        }
    }
    return true;
}

template <typename TryAddStringFunc>
bool print_string(va_list& args, TryAddStringFunc& try_add_string)
{
    // No such modifiers for const char*
    const char* str = va_arg(args, const char*);
    return try_add_string(str);
}

template <typename TryAddStringFunc>
bool print_char(va_list& args, TryAddStringFunc& try_add_string)
{
    // We'll ignore the long modifier here... wide chars is not
    // something we care about
    int arg_ch = va_arg(args, int);
    char str[2] = " ";
    str[0] = arg_ch;
    return try_add_string(str);
}

template <typename TryAddStringFunc>
bool print_int(char type_ch, va_list& args, TryAddStringFunc& try_add_string, ArgModifiers mod)
{
    char digits[12];

    if (type_ch == 'u') {
        unsigned long num;
        if (mod == ModLong)
            num = va_arg(args, unsigned long);
        else
            num = va_arg(args, unsigned int);

        ultoa10(digits, num);
    } else {
        long num;
        if (mod == ModLong)
            num = va_arg(args, long);
        else
            num = va_arg(args, int);

        ltoa10(digits, num);
    }

    return try_add_string(digits);
}

template <typename TryAddStringFunc>
bool print_pointer(va_list& args, TryAddStringFunc& try_add_string)
{
    void* ptr = va_arg(args, void*);
    auto ptr_data = (unsigned long)ptr;

    char hex[11];
    ultoa16(hex, ptr_data, ToALower);
    return try_add_string(hex);
}
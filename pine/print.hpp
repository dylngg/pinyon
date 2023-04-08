#pragma once

// Note: Headers such as <c...> comes from GCC's builtin functions when using
//       -ffrestanding: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
//       In clang with -ffreestanding, these are not defined, but the C ones
//       are.
#ifdef CLANG_HAS_NO_CXX_INCLUDES
#include <stdarg.h>
#else
#include <cstdarg>
#endif

#include "c_builtins.hpp"
#include "iter.hpp"
#include "limits.hpp"
#include "math.hpp"
#include "string_view.hpp"

/*
 * This header defines simple concatenation based printing of arguments to some
 * "print()" function. The particular name of this print function depends on the
 * environment. (kernel, userspace, alien)
 *
 * The format of each argument is based on the type of each argument. More
 * specifically, the formatting is implemented with an associated
 * print_with(Printer&, const Arg& arg) function. Class-specific formatting
 * may be implemented with a friend function, much like the STL and operator<<
 * with ostream.
 *
 * To abstract away the particular printing mechanism (UART/console, stdio)
 * from the implementation, a virtual Printer functor is used (much like
 * operator<< in the STL)
 *
 * See pine/alien/print.hpp for a reference implementation of print() and the
 * ARMv7 CPSR class in the kernel for a reference implementation of print_with().
 */

namespace pine {

class Printer {
public:
    virtual ~Printer() = default;

protected:
    virtual void print(StringView) = 0;
    friend void print_with(Printer& printer, StringView string);
};

template <typename First>
inline void print_each_with_spacing(Printer& printer, const First& first)
{
    print_with(printer, first);
}

template <typename First, typename... Args>
inline void print_each_with_spacing(Printer& printer, const First& first, const Args&... args)
{
    print_each_with_spacing(printer, first);
    print_each_with_spacing(printer, " ");
    print_each_with_spacing(printer, args...);
}

template <typename First>
inline void print_each_with(Printer& printer, const First& first)
{
    print_with(printer, first);
}

template <typename First, typename... Args>
inline void print_each_with(Printer& printer, const First& first, const Args&... args)
{
    print_each_with(printer, first);
    print_each_with(printer, args...);
}

template <typename Int, enable_if<is_integer<Int>, Int>* = nullptr>
void print_with(Printer& printer, Int num)
{
    constexpr int bufsize = limits<Int>::characters10 + 1;  // '\0'
    char buf[bufsize]; // FIXME: Replace with Buffer<>/Array<>
    bzero(buf, bufsize);

    to_strbuf(buf, bufsize, num);
    print_with(printer, buf);
}

template <typename Ptr, enable_if<is_pointer<Ptr> && !is_implicitly_convertible<Ptr, StringView>, Ptr>* = nullptr>
void print_with(Printer& printer, Ptr ptr)
{
    constexpr int bufsize = limits<uintptr_t>::digits16 + 3; // + 0x + '\0'
    char buf[bufsize]; // FIXME: Replace with Buffer<>/Array<>
    bzero(buf, bufsize);

    buf[0] = '0';
    buf[1] = 'x';

    to_strbuf_hex(buf + 2, bufsize, reinterpret_cast<uintptr_t>(ptr), ToAFlag::Lower);
    print_with(printer, buf);
}

inline void print_with(Printer& printer, StringView string)
{
    printer.print(string);
}

enum class FlagModifiers {
    None,
    AlternativeForm,
};

enum class ArgModifiers {
    None,
    Long,
    SizeT,
};

size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args);
size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...);

template <typename TryAddStringFunc>
size_t vfnprintf(TryAddStringFunc& try_add_string, const char* fmt, va_list rest)
{
    FlagModifiers flag;
    ArgModifiers mod;
    // Keep a fake two char string on the stack for easy adding of single or dual chars
    char chstr[2] = " ";

    auto fmt_view = StringView(fmt);
    for (auto iter = fmt_view.begin(); !iter.at_end(); ++iter) {
        auto ch = *iter;
        chstr[0] = ch;

        if (ch != '%') {
            // Literal
            try_add_string(chstr);
            continue;
        }

        // Format specifier: %s

        ++iter; // consume '%'
        flag = FlagModifiers::None;
        mod = ArgModifiers::None;

        char flag_ch = *iter;
        if (flag_ch == '#') {
            flag = FlagModifiers::AlternativeForm;
            ++iter;
        }

        char type_or_mod_ch = *iter;
        if (type_or_mod_ch == 'l') {
            mod = ArgModifiers::Long;
            ++iter; // consume 'l'
        } else if (type_or_mod_ch == 'z') {
            mod = ArgModifiers::SizeT;
            ++iter; // consume 'z'
        }

        // Sorry, but we have to have large switch cases:
        // https://stackoverflow.com/questions/8047362/is-gcc-mishandling-a-pointer-to-a-va-list-passed-to-a-function
        char type_ch = *iter;
        switch (type_ch) {
        case 's': {
            const char* str = va_arg(rest, const char*);
            if (!try_add_string(str))
                return false;
            break;
        }

        case '%':
            if (!try_add_string(chstr))
                return false;
            break;

        case 'c': {
            char str[2] = " ";
            str[0] = static_cast<char>(va_arg(rest, int));
            if (!try_add_string(str))
                return false;
            break;
        }

        case 'u': {
            constexpr int bufsize = limits<size_t>::characters10 + 1; // max int possible here
            char digits[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(digits, bufsize);

            switch (mod) {
            case ArgModifiers::Long:
                to_strbuf(digits, bufsize, va_arg(rest, unsigned long));
                break;
            case ArgModifiers::SizeT:
                to_strbuf(digits, bufsize, va_arg(rest, size_t));
                break;
            case ArgModifiers::None:
                to_strbuf(digits, bufsize, va_arg(rest, unsigned int));
                break;
            }

            if (!try_add_string(digits))
                return false;
            break;
        }
        case 'd': {
            constexpr int bufsize = limits<ssize_t>::characters10 + 1; // max int possible here
            char digits[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(digits, bufsize);

            switch (mod) {
            case ArgModifiers::Long:
                to_strbuf(digits, bufsize, va_arg(rest, long));
                break;
            case ArgModifiers::SizeT:
                // signed size_t; mostly to keep compiler happy about exhaustive switches
                static_assert(sizeof(ssize_t) == sizeof(size_t));
                to_strbuf(digits, bufsize, va_arg(rest, ssize_t));
                break;
            case ArgModifiers::None:
                to_strbuf(digits, bufsize, va_arg(rest, int));
                break;
            }

            if (!try_add_string(digits))
                return false;
            break;
        }

        case 'x': {
            constexpr int bufsize = limits<size_t>::digits16 + 3; // max int possible here; + 0x + \0
            char hex[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            char* hex_start = hex;
            size_t hex_bufsize = bufsize;
            bzero(hex, bufsize);

            if (flag == FlagModifiers::AlternativeForm) {
                hex[0] = '0';
                hex[1] = 'x';
                hex_start = hex + 2;
                hex_bufsize -= 2;
            }

            switch (mod) {
            case ArgModifiers::Long:
                to_strbuf_hex(hex_start, hex_bufsize, va_arg(rest, unsigned long), ToAFlag::Lower);
                break;
            case ArgModifiers::SizeT:
                to_strbuf_hex(hex_start, hex_bufsize, va_arg(rest, size_t), ToAFlag::Lower);
                break;
            case ArgModifiers::None:
                to_strbuf_hex(hex_start, hex_bufsize, va_arg(rest, unsigned int), ToAFlag::Lower);
                break;
            }

            if (!try_add_string(hex))
                return false;
            break;
        }

        case 'p': {
            PtrData ptr_data = va_arg(rest, PtrData);

            constexpr int bufsize = limits<PtrData>::digits16 + 3;  // 0x + \0
            char hex[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(hex, bufsize);

            hex[0] = '0';
            hex[1] = 'x';

            to_strbuf_hex(hex + 2, bufsize - 2, ptr_data, ToAFlag::Lower);
            if (!try_add_string(hex))
                return false;
            break;
        }

        default:
            // Bail here. The caller may have provided an argument and if we
            // proceed then future arguments may be misaligned...
            // FIXME: Perhaps there is a better way to deal with this...
            return false;
        }
    }
    return true;
}

}

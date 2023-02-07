#pragma once

#include <cstdarg>

#include "c_builtins.hpp"
#include "iter.hpp"
#include "limits.hpp"
#include "math.hpp"
#include "string_view.hpp"

/*
 * This header defines simple conatenation based printing of arguments to some
 * "print()" function. No spaces are inserted between arguments, unlike the
 * similar look to Python's print function. The particular name of this print
 * function depends on the environment. (kernel, userspace, alien)
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
inline void print_each_with(Printer& printer, const First& first)
{
    print_with(printer, first);
}

template <typename First, typename... Args>
void print_each_with(Printer& printer, const First& first, const Args&... args)
{
    print_each_with(printer, first);
    print_each_with(printer, " ");
    print_each_with(printer, args...);
}

template <typename Int, enable_if<is_integer<Int>, Int>* = nullptr>
void print_with(Printer& printer, Int num)
{
    constexpr int bufsize = limits<Int>::characters + 1;  // + '-' and '\0'
    char buf[bufsize]; // FIXME: Replace with Buffer<>/Array<>
    bzero(buf, bufsize);

    to_strbuf(buf, bufsize, num);
    print_with(printer, buf);
}

template <typename Ptr, enable_if<is_pointer<Ptr> && !is_implicitly_convertible<Ptr, StringView>, Ptr>* = nullptr>
void print_with(Printer& printer, Ptr ptr)
{
    constexpr int bufsize = limits<uintptr_t>::characters + 3; // + 0x + '\0'
    char buf[bufsize]; // FIXME: Replace with Buffer<>/Array<>
    bzero(buf, bufsize);

    to_strbuf_hex(buf, bufsize, reinterpret_cast<uintptr_t>(ptr), ToAFlag::Lower);
    print_with(printer, buf);
}

inline void print_with(Printer& printer, StringView string)
{
    printer.print(string);
}

enum class ArgModifiers {
    None,
    Long,
    LongLong,
    SizeT,
};

size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args);
size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...);

template <typename TryAddStringFunc>
size_t vfnprintf(TryAddStringFunc& try_add_string, const char* fmt, va_list rest)
{
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
        mod = ArgModifiers::None;

        char type_or_mod_ch = *iter;
        if (type_or_mod_ch == 'l') {
            mod = ArgModifiers::Long;
            ++iter; // consume 'l'
            if (*iter == 'l') {
                mod = ArgModifiers::LongLong;
                ++iter;
            }
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

        case 'u':
            [[fallthrough]];
        case 'd': {
            constexpr int bufsize = limits<unsigned long long>::digits + 1; // max int possible here
            char digits[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(digits, bufsize);

            if (type_ch == 'u') {
                switch (mod) {
                case ArgModifiers::LongLong:
                    to_strbuf(digits, bufsize, va_arg(rest, unsigned long long));
                    break;
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
            } else {
                switch (mod) {
                case ArgModifiers::LongLong:
                    to_strbuf(digits, bufsize, va_arg(rest, long long));
                    break;
                case ArgModifiers::Long:
                    to_strbuf(digits, bufsize, va_arg(rest, long));
                    break;
                case ArgModifiers::SizeT:
                    // signed size_t; mostly to keep compiler happy about exhaustive switches
                    static_assert(sizeof(intptr_t) == sizeof(size_t));
                    to_strbuf(digits, bufsize, va_arg(rest, intptr_t));
                    break;
                case ArgModifiers::None:
                    to_strbuf(digits, bufsize, va_arg(rest, int));
                    break;
                }
            }

            if (!try_add_string(digits))
                return false;
            break;
        }

        case 'x': {
            constexpr int bufsize = limits<unsigned long long>::digits + 1; // max int possible here
            char hex[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(hex, bufsize);

            switch (mod) {
            case ArgModifiers::LongLong:
                to_strbuf_hex(hex, bufsize, va_arg(rest, unsigned long long), ToAFlag::Lower);
                break;
            case ArgModifiers::Long:
                to_strbuf_hex(hex, bufsize, va_arg(rest, unsigned long), ToAFlag::Lower);
                break;
            case ArgModifiers::SizeT:
                to_strbuf_hex(hex, bufsize, va_arg(rest, size_t), ToAFlag::Lower);
                break;
            case ArgModifiers::None:
                to_strbuf_hex(hex, bufsize, va_arg(rest, unsigned int), ToAFlag::Lower);
                break;
            }

            if (!try_add_string(hex))
                return false;
            break;
        }

        case 'p': {
            PtrData ptr_data = va_arg(rest, PtrData);

            constexpr int bufsize = limits<PtrData>::digits + 1;
            char hex[bufsize]; // FIXME: Replace with Buffer<>/Array<>
            bzero(hex, bufsize);

            to_strbuf_hex(hex, bufsize, ptr_data, ToAFlag::Lower);
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

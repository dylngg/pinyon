#include <pine/console.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>

void inline console_put(char ch)
{
    *(volatile size_t*)(UART0_BASE) = ch;
}

void console(const StringView& message)
{
    for (auto ch : message)
        console_put(ch);
}

void consoleln(const StringView& message)
{
    console(message);
    console_put('\n');
}

void consolef(const StringView& fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const auto& try_add_wrapper = [](const StringView& message) -> bool {
        console(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}
#include <pine/console.hpp>
#include <pine/string.hpp>

void inline console_put(char ch)
{
    *(volatile size_t*)(UART0_BASE) = ch;
}

void console(const String& message)
{
    for (auto ch : message)
        console_put(ch);
}

void consoleln(const String& message)
{
    console(message);
    console_put('\n');
}

void consolef(const String& fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const auto& try_add_wrapper = [](const String& message) -> bool {
        console(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}
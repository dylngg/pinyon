#pragma once
#include <pine/barrier.hpp>
#include <pine/print.hpp>
#include <pine/string_view.hpp>
#include <pine/types.hpp>
#include <pine/utility.hpp>

/*
 * Logging functions for the kernel; these poll the UART, instead of using IRQs
 * since we want the message to be pushed out before we return.
 */
class UARTPrinter : public pine::Printer {
public:
    void print(pine::StringView) override;
};

template <typename... Args>
inline void console(Args&& ... args)
{
    auto printer = UARTPrinter();
    print_each_with(printer, pine::forward<Args>(args)...);
}

template <typename... Args>
inline void consoleln(Args&& ... args)
{
    console(pine::forward<Args>(args)..., "\n");
}

void consolef(const char*, ...) __attribute__((format(printf, 1, 2)));

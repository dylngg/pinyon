#include "console.hpp"
#include "uart.hpp"

#include <pine/barrier.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>
#include <pine/types.hpp>

void console_readline(char* buf, size_t bufsize)
{
    MemoryBarrier barrier;
    UARTManager::ReadWriteInterruptMask mask {};

    auto& uart = uart_manager();
    size_t offset = 0;
    char ch;
    for (;;) {
        ch = uart.poll_get();

        if (offset >= bufsize - 1)
            continue;

        bool stop = false;
        switch (ch) {
        case '\r':
            stop = true;
            break;
        case '\n':
            stop = true;
            break;
        case '\t':
            // ignore; messes with delete :P
            continue;
        case 127:
            // delete
            if (offset >= 1) {
                offset--;
                // move left one char (D), then delete one (P)
                uart.poll_write("\x1b[1D\x1b[1P");
            }
            continue;
        default:
            // local echo
            uart.poll_put(ch);
        }
        if (stop)
            break;

        buf[offset] = ch;
        offset++;
    }

    uart.poll_put('\n');
    buf[offset] = '\0';
}

void console(const char* message)
{
    MemoryBarrier barrier;
    UARTManager::WriteInterruptMask mask {};
    uart_manager().poll_write(message);
}

void console(const char* message, size_t bytes)
{
    MemoryBarrier barrier;
    UARTManager::WriteInterruptMask mask {};
    uart_manager().poll_write(message, bytes);
}

void consoleln(const char* message)
{
    MemoryBarrier barrier;
    UARTManager::WriteInterruptMask mask {};

    auto& uart = uart_manager();
    uart.poll_write(message);
    uart.poll_put('\n');
}

void consolef(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    MemoryBarrier barrier;
    UARTManager::WriteInterruptMask mask {};

    auto& uart = uart_manager();
    const auto& try_add_wrapper = [&](const char* message) -> bool {
        uart.poll_write(message);
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
}

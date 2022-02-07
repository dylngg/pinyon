#include "console.hpp"
#include "uart.hpp"

#include <pine/barrier.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>
#include <pine/types.hpp>

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

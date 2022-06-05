#include "console.hpp"
#include "uart.hpp"

#include <pine/barrier.hpp>
#include <pine/printf.hpp>
#include <pine/string.hpp>
#include <pine/types.hpp>

void console(const char* message)
{
    pine::MemoryBarrier barrier;
    auto& uart = uart_registers();
    UARTRegisters::WriteInterruptMask mask { uart };

    uart.poll_write(message);
}

void consoleln(const char* message)
{
    pine::MemoryBarrier barrier;
    auto& uart = uart_registers();
    UARTRegisters::WriteInterruptMask mask { uart };

    uart.poll_write(message);
    uart.poll_put('\n');
}

void consolef(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pine::MemoryBarrier barrier;
    auto& uart = uart_registers();
    UARTRegisters::WriteInterruptMask mask { uart };

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        uart.poll_write(message);
        return true;
    };
    pine::vfnprintf(try_add_wrapper, fmt, args);
}

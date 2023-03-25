#include "console.hpp"
#include "device/pl011/uart.hpp"
#include "arch/barrier.hpp"

void UARTPrinter::print(pine::StringView message)
{
    pine::MemoryBarrier barrier;
    auto& uart = uart_registers();
    UARTRegisters::WriteInterruptMask mask { uart };

    uart.poll_write(message.data());
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

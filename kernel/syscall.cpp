#include "console.hpp"
#include "device/timer.hpp"
#include "syscall.hpp"
#include "tasks.hpp"

#include <pine/limits.hpp>
#include <pine/cast.hpp>

PtrData handle_syscall(Syscall call, PtrData arg1, PtrData arg2, PtrData arg3)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();
    auto conversion_error = from_signed_cast<PtrData>(1);

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (call) {
    case Syscall::Exit: {
        InterruptDisabler disabler;
        if (!fits_within<int>(arg1)) {
            return conversion_error;
        }
        int code = to_signed_cast<int>(arg1);
        task_mgr.exit_running_task(disabler, code);
        break;
    }

    case Syscall::Yield: {
        InterruptDisabler disabler;
        task_mgr.schedule(disabler);
        break;
    }

    case Syscall::Sleep:
        if (!fits_within<u32>(arg1)) {
            return conversion_error;
        }
        task.sleep(static_cast<u32>(arg1));
        break;

    case Syscall::Open: {
        auto arg1_char = reinterpret_cast<const char*>(arg1);
        auto arg2_signed = pine::bit_cast<ptrdiff_t>(arg2);
        static_assert(pine::is_signed<pine::underlying_type<FileMode>>);
        if (arg2_signed < static_cast<ptrdiff_t>(FileMode::Read) || arg2_signed > static_cast<ptrdiff_t>(FileMode::ReadWrite)) {
            return conversion_error;
        }
        int fd = task.open(arg1_char, static_cast<FileMode>(arg2_signed));
        return from_signed_cast<PtrData>(fd);
    }

    case Syscall::Read: {
        if (!fits_within<int>(arg1)) {
            return conversion_error;
        }
        auto ret = task.read(to_signed_cast<int>(arg1), reinterpret_cast<char*>(arg2), static_cast<size_t>(arg3));
        return from_signed_cast<PtrData>(ret);
    }

    case Syscall::Write: {
        if (!fits_within<int>(arg1)) {
            return conversion_error;
        }
        auto ret = task.write(to_signed_cast<int>(arg1), reinterpret_cast<char*>(arg2), static_cast<size_t>(arg3));
        return from_signed_cast<PtrData>(ret);
    }

    case Syscall::Close: {
        if (!fits_within<int>(arg1)) {
            return conversion_error;
        }
        auto ret = task.close(to_signed_cast<int>(arg1));
        return from_signed_cast<PtrData>(ret);
    }

    case Syscall::Dup: {
        if (!fits_within<int>(arg1)) {
            return conversion_error;
        }
        auto ret = task.dup(to_signed_cast<int>(arg1));
        return from_signed_cast<PtrData>(ret);
    }

    case Syscall::Sbrk:
        return reinterpret_cast<PtrData>(task.sbrk(static_cast<size_t>(arg1)));

    case Syscall::Uptime:
        return static_cast<PtrData>(jiffies());

    case Syscall::CPUTime:
        return static_cast<PtrData>(task.cputime());

    default:
        consoleln("kernel:\tUnknown syscall number ", static_cast<PtrData>(call));
    }

    return 0;
}


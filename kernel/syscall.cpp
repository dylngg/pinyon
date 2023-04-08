#include "console.hpp"
#include "device/timer.hpp"
#include "syscall.hpp"
#include "tasks.hpp"

#include <pine/cast.hpp>

pine::Maybe<Syscall> validate_syscall(PtrData call_data)
{
    static_assert(pine::is_signed<pine::underlying_type<Syscall>>);

    auto syscall_signed = pine::bit_cast<ptrdiff_t>(call_data);
    if (syscall_signed < static_cast<ptrdiff_t>(Syscall::Yield) || syscall_signed > static_cast<ptrdiff_t>(Syscall::Exit)) {
        return {};
    }
    return { static_cast<Syscall>(call_data) };
}

pine::Maybe<FileMode> validate_file_mode(PtrData file_mode_data)
{
    static_assert(pine::is_signed<pine::underlying_type<FileMode>>);

    auto arg2_signed = pine::bit_cast<ptrdiff_t>(file_mode_data);
    if (arg2_signed < static_cast<ptrdiff_t>(FileMode::Read) || arg2_signed > static_cast<ptrdiff_t>(FileMode::ReadWrite)) {
        return {};
    }
    return { static_cast<FileMode>(file_mode_data) };
}

PtrData handle_syscall(PtrData call_data, PtrData arg1, PtrData arg2, PtrData arg3)
{
    auto conversion_error = from_signed_cast<PtrData>(-1);

    auto maybe_syscall = validate_syscall(call_data);
    if (!maybe_syscall) {
        consoleln("kernel:\tUnknown syscall data ", call_data);
        return conversion_error;
    }

    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task(InterruptDisabler {});

    switch (*maybe_syscall) {
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
        auto maybe_file_mode = validate_file_mode(arg2);
        if (!maybe_file_mode) {
            return conversion_error;
        }
        int fd = task.open(arg1_char, *maybe_file_mode);
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
    }

    return 0;
}


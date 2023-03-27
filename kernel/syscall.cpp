#include "console.hpp"
#include "device/timer.hpp"
#include "syscall.hpp"
#include "tasks.hpp"

PtrData handle_syscall(Syscall call, PtrData arg1, PtrData arg2, PtrData arg3)
{
    auto& task_mgr = task_manager();
    auto& task = task_mgr.running_task();

    //consolef("Handling syscall %u with args %u\n", syscall_id, arg);

    switch (call) {
    case Syscall::Exit: {
        InterruptDisabler disabler;
        int code = static_cast<int>(arg1);
        task_mgr.exit_running_task(disabler, code);
        break;
    }

    case Syscall::Yield: {
        InterruptDisabler disabler;
        task_mgr.schedule(disabler);
        break;
    }

    case Syscall::Sleep:
        task.sleep(arg1);
        break;

    case Syscall::Open:
        return pine::bit_cast<PtrData>(task.open(reinterpret_cast<const char*>(arg1), pine::bit_cast<FileMode>(arg2)));

    case Syscall::Read:
        return pine::bit_cast<PtrData>(task.read(pine::bit_cast<int>(arg1), reinterpret_cast<char*>(arg2), pine::bit_cast<size_t>(arg3)));

    case Syscall::Write:
        return pine::bit_cast<PtrData>(task.write(pine::bit_cast<int>(arg1), reinterpret_cast<char*>(arg2), pine::bit_cast<size_t>(arg3)));

    case Syscall::Close:
        return pine::bit_cast<PtrData>(task.close(pine::bit_cast<int>(arg1)));

    case Syscall::Dup:
        return pine::bit_cast<PtrData>(task.dup(pine::bit_cast<int>(arg1)));

    case Syscall::Sbrk:
        return reinterpret_cast<PtrData>(task.sbrk(pine::bit_cast<size_t>(arg1)));

    case Syscall::Uptime:
        return jiffies();

    case Syscall::CPUTime:
        return task.cputime();

    default:
        consoleln("kernel:\tUnknown syscall number ", static_cast<PtrData>(call));
    }

    return 0;
}


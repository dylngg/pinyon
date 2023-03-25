#pragma once
#include "arch/aarch32/processor.hpp"
#include "stack.hpp"
#include "file.hpp"
#include "kmalloc.hpp"
#include "wait.hpp"
#include "interrupt_disabler.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/string.hpp>
#include <pine/string_view.hpp>
#include <pine/syscall.hpp>
#include <pine/types.hpp>
#include <pine/vector.hpp>

extern "C" {
// In bootup.S
u32 halt_addr();
// In switch.S
void task_switch(Registers* to_save_registers, bool is_kernel_task_save, const Registers* new_registers, bool is_kernel_task_new);
}

Registers construct_user_task_registers(const Stack& stack, const Stack& kernel_stack, PtrData pc);
Registers construct_kernel_task_registers(const Stack& kernel_stack, PtrData pc);

class Heap : public pine::FallbackAllocatorBinder<pine::FixedAllocation, pine::HighWatermarkAllocator> {
public:
    Heap(PtrData start, size_t size)
        : pine::FallbackAllocatorBinder<pine::FixedAllocation, pine::HighWatermarkAllocator>(&m_allocation)
        , m_allocation(start, size)
        , m_high_watermark_allocator() {};

    pine::FixedAllocation m_allocation;
    pine::HighWatermarkAllocator m_high_watermark_allocator;
};

class Task {
public:
    enum class State : int {
        New = 0,
        Runnable,
        Waiting,
    };
    enum CreateFlags {
        CreateUserTask = 0,
        CreateKernelTask = 1,
    };

    static pine::Maybe<Task> try_create(const char* name, u32 pc, CreateFlags flags);
    Task(const Task& other) = delete;
    Task(Task&& other) = default;
    Task& operator=(Task&& other) = default;

    const KString& name() const { return m_name; }
    void sleep(u32 secs);
    int open(pine::StringView path, FileMode mode);
    ssize_t read(int fd, char* buf, size_t at_most_bytes);
    ssize_t write(int fd, char* buf, size_t bytes);
    int close(int fd);
    int dup(int fd);
    u32 cputime();
    void* sbrk(size_t increase);

    void reschedule_while_waiting_for(const Waitable&);

    bool is_kernel_task() const { return m_registers.is_kernel_registers(); }

private:
    Task(KString name, Heap heap, Stack kernel_stack, pine::Maybe<Stack> user_stack, Registers registers, FileDescriptorTable fd_table);
    void update_state();
    void start(Registers*, bool is_kernel_task_to_save, InterruptsDisabledTag);
    void switch_to(Task&, InterruptsDisabledTag);
    friend class TaskManager;

    bool can_run() const { return m_state == State::New || m_state == State::Runnable; };

    class SleepWaitable : public Waitable {
    public:
        SleepWaitable(u32 secs);
        bool is_finished() const override;

    private:
        u32 m_sleep_end_time;
    };

    int userspace_buffer_is_valid(const char* buffer, size_t size);

    KString m_name;
    State m_state;
    pine::Maybe<Stack> m_user_stack;
    Stack m_kernel_stack;
    Registers m_registers;
    Heap m_heap;
    u32 m_jiffies_when_scheduled;
    u32 m_cpu_jiffies;
    const Waitable* m_waiting_for;
    FileDescriptorTable m_fd_table;
};

void reschedule_while_waiting_for(const Waitable&);

extern "C" {
[[noreturn]] void spin_task();
}

class TaskManager {
public:
    TaskManager();
    void start_scheduler(InterruptsDisabledTag);
    void schedule(InterruptsDisabledTag);
    void exit_running_task(InterruptsDisabledTag, int code);
    Task& running_task() { return m_tasks[m_running_task_index]; }

private:
    TaskManager(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    Task& pick_next_task();

    bool try_create_task(const char* name, PtrData start_addr, Task::CreateFlags flags);

    KVector<Task> m_tasks;
    unsigned m_running_task_index;
};

TaskManager& task_manager();

void tasks_init();

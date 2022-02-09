#pragma once
#include "interrupts.hpp"
#include "uart.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

extern "C" {
// These are in switch.S. They save, then resume, or start a task in one go.
// The fact that this is done in one operation is important: In switch_to(),
// GCC with optimizations may push values onto the stack that it will pop
// before calling task_resume() or task_start(). Our saving of registers onto
// the stack in some e.g. task_save(), before task_resume()/task_start() will
// then conflict with this pop operation by GCC. i.e. we cannot guarantee that
// the stack will not be manipulated before some save() and resume()/start().
void task_resume(u32* old_sp_ptr, u32 new_sp);
void task_start(u32* old_sp_ptr, u32 new_pc, u32 new_sp);
}

enum class TaskState : int {
    New = 0,
    Runnable,
    Sleeping,
    Waiting,
};

class TaskManager;

class Task {
public:
    Task(const char* name, u32 stack_pointer, u32 pc);
    ~Task();
    Task(const Task& other) = delete;
    Task(Task&& other) = delete;
    Task& operator=(const Task& other) = delete;
    Task& operator=(Task&& other) = delete;

    void sleep(u32 secs);
    size_t read(char* buf, size_t at_most_bytes);
    void write(char* buf, size_t bytes);
    u32 cputime();
    PtrData heap_allocate();
    PtrData heap_increase(size_t bytes);

    friend TaskManager;

private:
    void update_state();
    void start(u32*);
    void resume(u32*);
    void switch_to(Task& task, InterruptsDisabledTag);
    bool has_not_started() const { return m_state == TaskState::New; }
    bool is_waiting() const { return m_state == TaskState::Waiting; };
    bool can_run() const { return m_state == TaskState::New || m_state == TaskState::Runnable; };

    size_t make_uart_request(char* buf, size_t bytes, UARTResource::Options);

    u32 m_sp;
    u32 m_pc;
    u32 m_sleep_end_time;
    char* m_name;
    TaskState m_state;
    size_t m_heap_start;
    size_t m_heap_size;
    size_t m_heap_reserved;
    u32 m_jiffies_when_scheduled;
    u32 m_cpu_jiffies;
    Maybe<KOwner<UARTResource>> m_maybe_uart_resource;
};

extern "C" {
[[noreturn]] void spin_task();
}

class TaskManager {
public:
    TaskManager();
    void start_scheduler();
    void schedule(InterruptsDisabledTag) __attribute__((returns_twice));
    Task& running_task() { return m_tasks[m_running_task_index]; }

private:
    Task& pick_next_task();

    // When tasks ask to schedule themselves, it means that they have received
    // an async request from a SWI handler, which does not disable interrupts.
    // So create a nice wrapper around schedule() for Task so we don't have to
    // type this out for every syscall Task method we implement
    void schedule()
    {
        InterruptDisabler disabler;
        schedule(disabler);
    };
    friend class Task;

    Task* m_tasks;
    int m_num_tasks;
    int m_running_task_index;
};

TaskManager& task_manager();

void tasks_init();

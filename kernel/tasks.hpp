#pragma once
#include "interrupts.hpp"
#include "uart.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

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
    void start();
    void resume();
    void switch_to(Task& task, InterruptsDisabledTag);
    bool has_not_started() const { return m_state == TaskState::New; }
    bool is_waiting() const { return m_state == TaskState::Waiting; };
    bool can_run() const { return m_state == TaskState::New || m_state == TaskState::Runnable; };

    u32 m_sp;
    u32 m_pc;
    u32 m_sleep_period;
    u32 m_sleep_start_time;
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
    void schedule(InterruptsDisabledTag);
    Task& running_task() { return m_tasks[m_running_task_index]; }

private:
    Task& pick_next_task();
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

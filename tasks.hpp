#pragma once
#include "console.hpp"
#include "shell.hpp"
#include <pine/types.hpp>

enum TaskState {
    TaskNew,
    TaskRunnable,
    TaskSleeping,
};

class TaskManager;

class Task {
public:
    Task(const char* name, u32 stack_pointer, u32 pc);
    void sleep(u32 ms);
    void readline(char* buf, size_t at_most_bytes);
    void write(char* buf, size_t bytes);

    friend TaskManager;

private:
    void update_state();
    void start();
    void switch_to(Task& task);
    bool has_not_started() const { return m_state == TaskNew; }
    bool can_run() const { return m_state == TaskNew || m_state == TaskRunnable; };

    u32 m_sp;
    u32 m_pc;
    u32 m_sleep_period;
    u32 m_sleep_start_time;
    char* m_name;
    TaskState m_state;
};

extern "C" {
[[noreturn]] void spin_task();
}

class TaskManager {
public:
    TaskManager();
    void start_scheduler();
    void schedule();
    static TaskManager& manager();
    Task& running_task() { return m_tasks[m_running_task_index]; }

private:
    Task& pick_next_task();

    Task* m_tasks;
    int m_num_tasks;
    int m_running_task_index;
};

void tasks_init();
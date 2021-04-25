#pragma once
#include "shell.hpp"
#include <pine/types.hpp>

enum TaskState {
    TaskNew,
    TaskRunnable,
};

class Task {
public:
    Task(const char* name, u32 stack_pointer, u32 pc);
    void start();
    void switch_to(Task& task);
    bool has_not_started() const { return m_state == TaskNew; }

private:
    u32 m_sp;
    u32 m_pc;
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

private:
    Task& running_task() { return m_tasks[m_running_task_index]; }
    Task& pick_next_task()
    {
        // Round robben
        if (m_running_task_index >= m_num_tasks - 1)
            m_running_task_index = 0;
        else
            m_running_task_index++;
        return m_tasks[m_running_task_index];
    }

    /*
     * Give each task a MiB of memory.
     */
    Task* m_tasks;
    unsigned int m_num_tasks;
    unsigned int m_running_task_index;
};

void tasks_init();
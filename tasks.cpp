#include "tasks.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include <pine/string.hpp>

extern "C" {
// These are in switch.S. They either save, resume, or start a task.
void task_save(u32* old_sp_ptr);
void task_resume(u32 new_sp);
void task_start(u32 new_pc, u32 new_sp);
}

Task::Task(const char* name, u32 stack_pointer, u32 pc)
{
    m_sp = stack_pointer;
    m_pc = pc;
    m_state = TaskNew;
    m_name = (char*)kmalloc(sizeof *name * (strlen(name) + 1));
    strcpy(m_name, name);
}

void Task::switch_to(Task& to_run_task)
{
    task_save(&m_sp);

    if (to_run_task.has_not_started())
        to_run_task.start();
    else
        task_resume(to_run_task.m_sp);
}

void Task::start()
{
    m_state = TaskRunnable;
    task_start(m_pc, m_sp);
}

void TaskManager::schedule()
{
    InterruptDisabler disabler {};

    auto& curr_task = running_task();

    auto& to_run_task = pick_next_task();
    if (&to_run_task == &curr_task)
        return;

    curr_task.switch_to(to_run_task);
}

static TaskManager g_task_manager;

TaskManager& TaskManager::manager()
{
    return g_task_manager;
}

void TaskManager::start_scheduler()
{
    m_tasks[0].start();
}

extern "C" {
[[noreturn]] void spin_task()
{
top:
    asm volatile("swi 0");
    goto top;
}
}

TaskManager::TaskManager()
{
    m_tasks = (Task*)kmalloc(2 * sizeof *m_tasks);

    // The compiler will literally give us null if we try and get the address
    // of a function via (void*) or (u32) casts... undefined behavior?
    //
    // Well anyways this is our hack
    u32 spin_task_addr;
    u32 shell_task_addr;
    asm volatile("ldr %0, =spin_task"
                 : "=r"(spin_task_addr));
    asm volatile("ldr %0, =shell"
                 : "=r"(shell_task_addr));

    m_tasks[0] = Task("shell", 0x3EF00000, shell_task_addr);
    m_tasks[1] = Task("spin", 0x3F000000, spin_task_addr);
    m_num_tasks = 2;
    m_running_task_index = 0;
}

void tasks_init()
{
    TaskManager manager {};
    g_task_manager = manager;
    g_task_manager.start_scheduler();
}

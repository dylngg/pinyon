#include "tasks.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "timer.hpp"
#include <pine/barrier.hpp>
#include <pine/string.hpp>
#include <pine/units.hpp>

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
    m_sleep_period = 0;
    m_sleep_start_time = 0;
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

void Task::sleep(u32 secs)
{
    m_state = TaskSleeping;
    m_sleep_start_time = jiffies();
    m_sleep_period = secs;
}

void Task::readline(char* buf, size_t at_most_bytes)
{
    console_readline(buf, at_most_bytes);
}

void Task::write(char* buf, size_t bytes)
{
    console(buf, bytes);
}

PtrData Task::heap_allocate()
{
    m_heap_size = 4 * MiB; // Fixed; cannot change so be liberal
    size_t top_addr = KernelMemoryBounds::bounds().try_reserve_topdown_space(m_heap_size);
    m_heap_start = top_addr - m_heap_size;
    m_heap_reserved = 0;
    return m_heap_start;
}

PtrData Task::heap_increase(size_t bytes)
{
    size_t incr = bytes;
    if (m_heap_reserved + incr > m_heap_size) {
        incr = m_heap_size - m_heap_reserved;
    }

    m_heap_reserved += incr;
    return incr;
}

void Task::update_state()
{
    if (m_state == TaskSleeping && m_sleep_start_time + m_sleep_period <= jiffies()) {
        m_state = TaskRunnable;
        m_sleep_start_time = 0;
        m_sleep_period = 0;
    }
}

Task& TaskManager::pick_next_task()
{
    do {
        // Round robben
        m_running_task_index++;
        if (m_running_task_index >= m_num_tasks)
            m_running_task_index = 0;

        m_tasks[m_running_task_index].update_state();
    } while (!m_tasks[m_running_task_index].can_run());

    return m_tasks[m_running_task_index];
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

TaskManager& TaskManager::manager()
{
    static TaskManager g_task_manager {};
    MemoryBarrier::sync();
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
    asm volatile("swi 0"); // yield
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

    PtrData shell_stack_start = KernelMemoryBounds::bounds().try_reserve_topdown_space(1 * MiB);
    m_tasks[0] = Task("shell", shell_stack_start, shell_task_addr);

    // The idea behind this task is that it will always be runnable so we
    // never have to deal with no runnable tasks. It will spin of course,
    // which is not ideal :P
    PtrData spin_stack_start = KernelMemoryBounds::bounds().try_reserve_topdown_space(Page);
    m_tasks[1] = Task("spin", spin_stack_start, spin_task_addr);

    m_num_tasks = 2;
    m_running_task_index = 0;
}

void tasks_init()
{
    TaskManager::manager().start_scheduler();
}

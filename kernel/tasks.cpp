#include "tasks.hpp"
#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"
#include "timer.hpp"

#include <pine/string.hpp>
#include <pine/units.hpp>

Task::Task(const char* name, u32 stack_pointer, u32 pc)
    : m_sp(stack_pointer)
    , m_pc(pc)
    , m_sleep_period(0)
    , m_sleep_start_time(0)
    , m_state(TaskState::New)
    , m_heap_start(0)
    , m_heap_size(0)
    , m_heap_reserved(0)
    , m_jiffies_when_scheduled(0)
    , m_cpu_jiffies(0)
{
    m_name = (char*)kmalloc(sizeof *name * (strlen(name) + 1));
    strcopy(m_name, name);
}

Task::~Task()
{
    kfree(m_name);
}

void Task::switch_to(Task& to_run_task, InterruptsDisabledTag)
{
    m_cpu_jiffies += jiffies() - m_jiffies_when_scheduled;
    if (to_run_task.has_not_started())
        to_run_task.start(&m_sp);
    else
        to_run_task.resume(&m_sp);
}

void Task::start(u32* prev_task_sp_ptr)
{
    m_state = TaskState::Runnable;
    m_jiffies_when_scheduled = jiffies();
    task_start(prev_task_sp_ptr, m_pc, m_sp);
}

void Task::resume(u32* prev_task_sp_ptr)
{
    m_jiffies_when_scheduled = jiffies();
    task_resume(prev_task_sp_ptr, m_sp);
}

void Task::sleep(u32 secs)
{
    m_state = TaskState::Sleeping;
    m_sleep_start_time = jiffies();
    m_sleep_period = secs * SYS_HZ;

    task_manager().schedule();

    m_sleep_start_time = 0;
    m_sleep_period = 0;
}

size_t Task::make_uart_request(char* buf, size_t bytes, UARTResource::Options options)
{
    auto maybe_resource = UARTResource::try_request(buf, bytes, options);
    PANIC_IF(!maybe_resource, "UART request failed!");

    if (maybe_resource->is_finished()) // If could be fulfilled without an IRQ
        return maybe_resource->size();

    // update_state(), called in pick_next_task() is what checks for whether
    // the resource is finished; it needs the resource
    m_maybe_uart_resource = move(maybe_resource);

    m_state = TaskState::Waiting;
    task_manager().schedule();

    PANIC_IF(!m_maybe_uart_resource->is_finished(), "Scheduled while UART resource is not finished!");
    maybe_resource = move(m_maybe_uart_resource);
    return maybe_resource->size();
}

size_t Task::read(char* buf, size_t at_most_bytes)
{
    UARTResource::Options options {
        false,
        true,
    };
    return make_uart_request(buf, at_most_bytes, options);
}

void Task::write(char* buf, size_t bytes)
{
    UARTResource::Options options {
        true,
        false,
    };
    make_uart_request(buf, bytes, options);
}

PtrData Task::heap_allocate()
{
    m_heap_size = 4 * MiB; // Fixed; cannot change so be liberal
    auto maybe_top_addr = kmem_bounds().try_reserve_topdown_space(m_heap_size);
    PANIC_IF(!maybe_top_addr, "Cannot reserve memory for task heap!");

    m_heap_start = maybe_top_addr.value() - m_heap_size;
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
    if (m_state == TaskState::Sleeping && m_sleep_start_time + m_sleep_period <= jiffies()) {
        m_state = TaskState::Runnable;
    }
    if (m_state == TaskState::Waiting && m_maybe_uart_resource && m_maybe_uart_resource->is_finished()) {
        m_state = TaskState::Runnable;
    }
}

u32 Task::cputime()
{
    if (&(task_manager().running_task()) == this) // if we're scheduled
        return m_cpu_jiffies + jiffies() - m_jiffies_when_scheduled;

    return m_cpu_jiffies;
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

void TaskManager::schedule(InterruptsDisabledTag disabled_tag)
{
    auto& curr_task = running_task();

    auto& to_run_task = pick_next_task();
    if (&to_run_task == &curr_task)
        return;

    curr_task.switch_to(to_run_task, disabled_tag);
}

void TaskManager::start_scheduler()
{
    // When we start, we save the previous task's SP into the task object; we
    // are starting afresh, so just put it in a dummy variable
    u32 dummy_old_sp;
    m_tasks[0].start(&dummy_old_sp);
}

extern "C" {
[[noreturn]] void spin_task()
{
top:
    asm volatile("wfi");
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

    auto maybe_shell_stack_start = kmem_bounds().try_reserve_topdown_space(1 * MiB);
    PANIC_IF(!maybe_shell_stack_start, "Cannot allocate memory for shell stack!");

    new (&m_tasks[0]) Task("shell", maybe_shell_stack_start.value(), shell_task_addr);

    // The idea behind this task is that it will always be runnable so we
    // never have to deal with no runnable tasks. It will spin of course,
    // which is not ideal :P
    auto maybe_spin_stack_start = kmem_bounds().try_reserve_topdown_space(Page);
    PANIC_IF(!maybe_shell_stack_start, "Cannot allocate memory for spin stack!");

    new (&m_tasks[1]) Task("spin", maybe_spin_stack_start.value(), spin_task_addr);

    m_num_tasks = 2;
    m_running_task_index = 0;
}

TaskManager& task_manager()
{
    static TaskManager g_task_manager {};
    return g_task_manager;
}

void tasks_init()
{
    task_manager().start_scheduler();
}

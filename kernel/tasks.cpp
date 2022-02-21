#include "tasks.hpp"
#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"
#include "timer.hpp"

#include <pine/string.hpp>
#include <pine/units.hpp>

Task::Task(const char* name, Heap heap, u32 stack_pointer, u32 pc)
    : m_sp(stack_pointer)
    , m_pc(pc)
    , m_sleep_end_time(0)
    , m_name(name)
    , m_state(State::New)
    , m_heap(heap)
    , m_jiffies_when_scheduled(0)
    , m_cpu_jiffies(0)
    , m_maybe_uart_resource()
{
}

Maybe<Task> Task::try_construct(const char* name, u32 pc)
{
    size_t stack_size = 4 * MiB;
    auto* stack_ptr = kmalloc(stack_size);
    if (!stack_ptr)
        return {};

    auto sp = reinterpret_cast<u32>(stack_ptr) + stack_size;

    size_t heap_size = 4 * MiB;
    auto* heap_ptr = kmalloc(heap_size);
    if (!heap_ptr)
        return {};

    auto heap_ptr_data = reinterpret_cast<PtrData>(heap_ptr);
    Heap heap = Heap::construct(heap_ptr_data, heap_ptr_data + heap_size);

    return Task { name, heap, sp, pc };
}

void Task::switch_to(Task& to_run_task, InterruptsDisabledTag tag)
{
    m_cpu_jiffies += jiffies() - m_jiffies_when_scheduled;
    if (to_run_task.has_not_started())
        to_run_task.start(&m_sp, tag);
    else
        to_run_task.resume(&m_sp, tag);
}

void Task::start(u32* prev_task_sp_ptr, InterruptsDisabledTag)
{
    m_state = State::Runnable; // move away from New state
    m_jiffies_when_scheduled = jiffies();
    task_start(prev_task_sp_ptr, m_pc, m_sp);
}

void Task::resume(u32* prev_task_sp_ptr, InterruptsDisabledTag)
{
    m_jiffies_when_scheduled = jiffies();
    task_resume(prev_task_sp_ptr, m_sp);
}

void Task::sleep(u32 secs)
{
    m_state = State::Sleeping;
    m_sleep_end_time = jiffies() + secs * SYS_HZ;

    reschedule();

    m_sleep_end_time = 0;
}

size_t Task::make_uart_request(char* buf, size_t bytes, UARTResource::Options options)
{
    auto maybe_resource = UARTResource::try_request(buf, bytes, options);
    PANIC_MESSAGE_IF(!maybe_resource, "UART request failed!");

    if (maybe_resource->is_finished()) // If could be fulfilled without an IRQ
        return maybe_resource->size();

    // update_state(), called in pick_next_task() is what checks for whether
    // the resource is finished; it needs the resource
    m_maybe_uart_resource = move(maybe_resource);

    m_state = State::Waiting;
    reschedule();

    PANIC_IF(!m_maybe_uart_resource->is_finished());
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

void* Task::heap_increase(size_t bytes)
{
    auto [ptr, _] = m_heap.allocate(bytes);
    return ptr;
}

void Task::update_state()
{
    if (m_state == State::Sleeping && m_sleep_end_time <= jiffies()) {
        m_state = State::Runnable;
    }
    if (m_state == State::Waiting && m_maybe_uart_resource && m_maybe_uart_resource->is_finished()) {
        m_state = State::Runnable;
    }
}

u32 Task::cputime()
{
    if (&(task_manager().running_task()) == this) // if we're scheduled
        return m_cpu_jiffies + jiffies() - m_jiffies_when_scheduled;

    return m_cpu_jiffies;
}

void Task::reschedule()
{
    InterruptDisabler disabler {};
    task_manager().schedule(disabler);
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

void TaskManager::start_scheduler(InterruptsDisabledTag disabled_tag)
{
    // When we start, we save the previous task's SP into the task object; we
    // are starting afresh, so just put it in a dummy variable
    u32 dummy_old_sp;
    m_tasks[0].start(&dummy_old_sp, disabled_tag);
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
    PANIC_MESSAGE_IF(!m_tasks, "Could not find memory for initial tasks?!");

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

    auto maybe_task = Task::try_construct("shell", shell_task_addr);
    PANIC_MESSAGE_IF(!maybe_task, "Could not construct shell task! Out of memory?!");
    new (&m_tasks[0]) Task(move(*maybe_task));

    // The idea behind this task is that it will always be runnable so we
    // never have to deal with no runnable tasks. It will spin of course,
    // which is not ideal :P
    maybe_task = Task::try_construct("spin", spin_task_addr);
    PANIC_MESSAGE_IF(!maybe_task, "Could not construct spin task! Out of memory?!");
    new (&m_tasks[1]) Task(move(*maybe_task));

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
    InterruptDisabler disabler {};
    interrupt_registers().enable_timer();
    task_manager().start_scheduler(disabler);
}

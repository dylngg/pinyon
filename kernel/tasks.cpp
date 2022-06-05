#include "tasks.hpp"
#include "console.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"
#include "timer.hpp"

#include <pine/string.hpp>
#include <pine/units.hpp>

Task::Task(const char* name, Heap heap, Stack kernel_stack, Stack stack, u32 user_pc, ProcessorMode user_mode)
    : m_name(name)
    , m_state(State::New)
    , m_kernel_stack(move(kernel_stack))
    , m_stack(move(stack))
    , m_registers(m_stack.sp(), m_kernel_stack.sp(), user_pc, user_mode)
    , m_heap(heap)
    , m_sleep_end_time(0)
    , m_jiffies_when_scheduled(0)
    , m_cpu_jiffies(0)
    , m_maybe_uart_resource()
{
}

Maybe<Task> Task::try_create(const char* name, u32 pc)
{
    auto maybe_kernel_stack = Stack::try_create(8 * Page);
    if (!maybe_kernel_stack)
        return {};

    auto maybe_stack = Stack::try_create(2 * MiB);
    if (!maybe_stack)
        return {};

    size_t heap_size = 4 * MiB;
    auto* heap_ptr = kmalloc(heap_size);
    if (!heap_ptr)
        return {};

    auto heap_ptr_data = reinterpret_cast<PtrData>(heap_ptr);
    Heap heap = Heap::construct(heap_ptr_data, heap_ptr_data + heap_size);

    return Task {
        name,
        heap,
        move(*maybe_kernel_stack),
        move(*maybe_stack),
        pc,
        ProcessorMode::User
    };
}

void Task::switch_to(Task& to_run_task, InterruptsDisabledTag tag)
{
    m_cpu_jiffies += jiffies() - m_jiffies_when_scheduled;
    to_run_task.start(&m_registers, tag);
}

void Task::start(Registers* to_save_registers, InterruptsDisabledTag)
{
    m_state = State::Runnable;  // move away from New state if new
    m_jiffies_when_scheduled = jiffies();

    task_switch(to_save_registers, &m_registers);
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

void* Task::sbrk(size_t increase)
{
    auto [ptr, _] = m_heap.allocate(increase);
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
    m_tasks[0].start(nullptr, disabled_tag);
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
    m_tasks = static_cast<Task*>(kmalloc(2 * sizeof *m_tasks));
    PANIC_MESSAGE_IF(!m_tasks, "Could not find memory for initial tasks?!");

    // The compiler will literally give us null if we try and get the address
    // of a function via (void*) or (u32) casts... undefined behavior?
    //
    // Well anyways this is our hack
    u32 spin_task_addr;
    u32 shell_task_addr;
    asm volatile("ldr %0, 1f"
                 : "=r"(spin_task_addr));  // 1f -> 1: at end of func
    asm volatile("ldr %0, 2f"
                 : "=r"(shell_task_addr));  // 2f -> 2: at end of func

    auto maybe_task = Task::try_create("shell", shell_task_addr);
    PANIC_MESSAGE_IF(!maybe_task, "Could not create shell task! Out of memory?!");
    new (&m_tasks[0]) Task(move(*maybe_task));

    // The idea behind this task is that it will always be runnable so we
    // never have to deal with no runnable tasks. It will spin of course,
    // which is not ideal :P
    maybe_task = Task::try_create("spin", spin_task_addr);
    PANIC_MESSAGE_IF(!maybe_task, "Could not create spin task! Out of memory?!");
    new (&m_tasks[1]) Task(move(*maybe_task));

    m_num_tasks = 2;
    m_running_task_index = 0;

    /* Place relative ldr pool behind this code */
    asm volatile("1: .word  spin_task");
    asm volatile("2: .word  shell");
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

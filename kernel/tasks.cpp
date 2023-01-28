#include "tasks.hpp"
#include "interrupts.hpp"
#include "kmalloc.hpp"
#include "panic.hpp"
#include "timer.hpp"

#include <pine/units.hpp>

Registers construct_user_task_registers(const Stack& stack, const Stack& kernel_stack, PtrData pc)
{
    return Registers(stack.sp(), kernel_stack.sp(), pc, ProcessorMode::User);
}

Registers construct_kernel_task_registers(const Stack& kernel_stack, PtrData pc)
{
    auto registers = Registers(kernel_stack.sp(), kernel_stack.sp(), pc, ProcessorMode::Supervisor);
    PANIC_IF(!registers.is_kernel_registers());
    return registers;
}

Task::Task(KString name, Heap heap, Stack kernel_stack, pine::Maybe<Stack> user_stack, Registers registers)
    : m_name(pine::move(name))
    , m_state(State::New)
    , m_user_stack(pine::move(user_stack))
    , m_kernel_stack(pine::move(kernel_stack))
    , m_registers(registers)
    , m_heap(heap)
    , m_sleep_end_time(0)
    , m_jiffies_when_scheduled(0)
    , m_cpu_jiffies(0)
    , m_maybe_uart_resource()
{
}

pine::Maybe<Task> Task::try_create(const char* name, u32 pc, CreateFlags flags)
{
    auto maybe_kernel_stack = Stack::try_create(8 * PageSize);
    if (!maybe_kernel_stack)
        return {};

    pine::Maybe<Registers> registers;  // delay initialization
    pine::Maybe<Stack> maybe_stack;

    if (!(flags & CreateKernelTask)) {
        maybe_stack = Stack::try_create(2 * MiB);
        if (!maybe_stack)
            return {};

        registers = construct_user_task_registers(*maybe_stack, *maybe_kernel_stack, pc);
    }
    else {
        registers = construct_kernel_task_registers(*maybe_kernel_stack, pc);
    }

    size_t heap_size = 4 * MiB;
    auto heap_alloc = kmalloc(heap_size);
    if (!heap_alloc)
        return {};

    auto heap_ptr_data = reinterpret_cast<PtrData>(heap_alloc.ptr);
    Heap heap(heap_ptr_data, heap_ptr_data + heap_size);

    auto maybe_name = KString::try_create(kernel_allocator(), name);
    if (!maybe_name)
        return {};

    return Task {
        pine::move(*maybe_name),
        heap,
        pine::move(*maybe_stack),
        pine::move(*maybe_kernel_stack),
        *registers,
    };
}

void Task::switch_to(Task& to_run_task, InterruptsDisabledTag tag)
{
    m_cpu_jiffies += jiffies() - m_jiffies_when_scheduled;
    to_run_task.start(&m_registers, is_kernel_task(), tag);
}

void Task::start(Registers* to_save_registers, bool is_kernel_task_to_save, InterruptsDisabledTag)
{
    m_state = State::Runnable;  // move away from New state if new
    m_jiffies_when_scheduled = jiffies();

    task_switch(to_save_registers, is_kernel_task_to_save, &m_registers, is_kernel_task());
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
    m_maybe_uart_resource = pine::move(maybe_resource);

    m_state = State::Waiting;
    reschedule();

    PANIC_IF(!m_maybe_uart_resource->is_finished());
    maybe_resource = pine::move(m_maybe_uart_resource);
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
        if (m_running_task_index >= m_tasks.length())
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
    m_tasks[0].start(nullptr, false, disabled_tag);
}

extern "C" {
u32 spin_addr();
[[noreturn]] void spin();
u32 shell_addr(); // forward declare; in userspace/shell.hpp
}

TaskManager::TaskManager()
    : m_tasks(kernel_allocator())
    , m_running_task_index(0)
{
    // The compiler will literally give us null if we try and get the address
    // of a function via (void*) or (u32) casts... undefined behavior?
    //
    // Well anyways this is our hack
    auto spin_task_addr = spin_addr();
    auto shell_task_addr = shell_addr();

    PANIC_MESSAGE_IF(!try_create_task("shell", shell_task_addr, Task::CreateUserTask), "Could not create shell task! Out of memory?!");

    // The idea behind this task is that it will always be runnable so we
    // never have to deal with no runnable tasks. It will spin of course,
    // which is not ideal :P
    PANIC_MESSAGE_IF(!try_create_task("spin", spin_task_addr, Task::CreateKernelTask), "Could not create spin task! Out of memory?!");
}

bool TaskManager::try_create_task(const char* name, PtrData start_addr, Task::CreateFlags flags)
{
    auto maybe_task = Task::try_create(name, start_addr, flags);
    if (!maybe_task)
        return false;

    m_tasks.append(pine::move(*maybe_task));
    return true;
}

void TaskManager::exit_running_task(InterruptsDisabledTag disabler, int code)
{
    consoleln(running_task().name(), "has exited with code:", code);
    m_tasks.remove(m_running_task_index);
    pick_next_task().start(nullptr, false, disabler);
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

#pragma once
#include "interrupts.hpp"
#include "uart.hpp"
#include "processor.hpp"
#include "stack.hpp"

#include <pine/maybe.hpp>
#include <pine/memory.hpp>
#include <pine/types.hpp>

extern "C" {
// In bootup.S
u32 halt_addr();
}

struct Registers {
    explicit Registers(u32 user_sp, u32 kernel_sp, u32 user_pc, ProcessorMode user_mode)
        : cpsr(user_mode)
        , user_sp(user_sp)
        , user_lr(0)
        , kernel_sp(kernel_sp)
        , kernel_lr(0)
        , r0(0)
        , r1(0)
        , r2(0)
        , r3(0)
        , r4(0)
        , r5(0)
        , r6(0)
        , r7(0)
        , r8(0)
        , r9(0)
        , r10(0)
        , r11(0)
        , r12(0)
        , pc(user_pc)
    {
        auto stop_addr = halt_addr();
        user_lr = stop_addr;
        kernel_lr = stop_addr;
    };

    CPSR cpsr;  // The CPSR to return to (either user or kernel, depending on if starting a task)
    u32 user_sp;
    u32 user_lr;
    u32 kernel_sp;
    u32 kernel_lr;
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    u32 r11;
    u32 r12;
    u32 pc;     // The PC to return to (either user or kernel, depending on if starting a task)
};

using IsSecondReturn = bool;

extern "C" {
// In switch.S
void task_switch(Registers* to_save_registers, const Registers* new_registers);
}

class Task {
    using Heap = pine::MemoryAllocator<pine::FixedAllocation, pine::HighWatermarkManager>;

public:
    enum class State : int {
        New = 0,
        Runnable,
        Sleeping,
        Waiting,
    };

    static pine::Maybe<Task> try_create(const char* name, u32 pc);
    Task(const Task& other) = delete;
    Task(Task&& other) = default;

    void sleep(u32 secs);
    size_t read(char* buf, size_t at_most_bytes);
    void write(char* buf, size_t bytes);
    u32 cputime();
    void* sbrk(size_t increase);

private:
    Task(const char* name, Heap heap, Stack kernel_stack, Stack stack, u32 pc, ProcessorMode mode);
    void update_state();
    void start(Registers*, InterruptsDisabledTag);
    void switch_to(Task&, InterruptsDisabledTag);
    friend class TaskManager;

    bool can_run() const { return m_state == State::New || m_state == State::Runnable; };

    size_t make_uart_request(char* buf, size_t bytes, UARTResource::Options);

    // When tasks ask to schedule themselves, it means that they have received
    // an async request from a SWI handler, which does not disable interrupts.
    // So create a nice wrapper around schedule() for Task so we don't have to
    // enable interrupts in every syscall.
    static void reschedule();

    const char* m_name; // must be static pointer!
    State m_state;
    Stack m_kernel_stack;
    Stack m_stack;
    Registers m_registers;
    Heap m_heap;
    u32 m_sleep_end_time;
    u32 m_jiffies_when_scheduled;
    u32 m_cpu_jiffies;
    pine::Maybe<KOwner<UARTResource>> m_maybe_uart_resource;
};

extern "C" {
[[noreturn]] void spin_task();
}

class TaskManager {
public:
    TaskManager();
    void start_scheduler(InterruptsDisabledTag);
    void schedule(InterruptsDisabledTag);
    Task& running_task() { return m_tasks[m_running_task_index]; }

private:
    TaskManager(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    Task& pick_next_task();

    Task* m_tasks;
    int m_num_tasks;
    int m_running_task_index;
};

TaskManager& task_manager();

void tasks_init();

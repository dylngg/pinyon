#include "lib.hpp"
#include "kmalloc.hpp"

void readline(char* buf, u32 at_most_bytes)
{
    syscall_readline(buf, at_most_bytes);
}

void yield()
{
    syscall_yield();
}

void sleep(u32 secs)
{
    syscall_sleep(secs);
}

__attribute__((format(printf, 1, 2))) void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t bufsize = strlen(fmt) * 2;
    char* print_buf = (char*)kmalloc(bufsize);
    size_t print_buf_pos = 0;

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        size_t message_size = strlen(message);
        if (print_buf_pos + message_size >= bufsize) {
            // realloc buffer size (hack)
            // FIXME: Implement realloc
            kfree(print_buf);
            bufsize = bufsize + message_size + 64;
            print_buf = (char*)kmalloc(bufsize);
        }

        strcpy(print_buf + print_buf_pos, message);
        print_buf_pos += message_size;
        return true;
    };
    vfnprintf(try_add_wrapper, fmt, args);
    syscall_write(print_buf, print_buf_pos);
}

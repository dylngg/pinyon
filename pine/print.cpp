#include "print.hpp"

namespace pine {

size_t vsbufprintf(char* buf, size_t bufsize, const char* fmt, va_list args)
{
    size_t buf_pos = 0;
    if (bufsize == 0)
        return 0;

    const auto& try_add_wrapper = [&](const char* message) -> bool {
        size_t message_size = strlen(message);
        size_t copied = strbufcopy(buf + buf_pos, bufsize - buf_pos, message);
        buf_pos += copied;
        return copied == message_size;
    };

    vfnprintf(try_add_wrapper, fmt, args);
    return buf_pos;
}

size_t sbufprintf(char* buf, size_t bufsize, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vsbufprintf(buf, bufsize, fmt, args);
}

}

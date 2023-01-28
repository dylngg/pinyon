#include "c_string.hpp"

// Note: this magic header comes from GCC's builtin functions
//       https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#Other-Builtins
#include <cstdarg>
#include "math.hpp"
#include "types.hpp"

namespace pine {

size_t strcopy(char* __restrict__ to, const char* from)
{
    size_t copied = 0;
    while (*from != '\0') {
        *to = *from;
        to++;
        from++;
        copied++;
    }
    *to = '\0';
    return copied;
}

size_t strbufcopy(char* __restrict__ buf, size_t bufsize, const char* from)
{
    if (bufsize == 0)
        return 0;

    size_t copied = 0;
    while (*from != '\0' && copied + 1 < bufsize) { // + '\0'
        *buf = *from;
        buf++;
        from++;
        copied++;
    }
    *buf = '\0';
    return copied;
}

size_t strlen(const char* string)
{
    size_t offset = 0;
    while (string[offset] != '\0') {
        offset++;
    }
    return offset;
}

int strcmp(const char* first, const char* second)
{
    size_t offset = 0;
    while (first[offset] != '\0' && second[offset] != '\0') {
        int diff = (unsigned char)first[offset] - (unsigned char)second[offset];
        if (diff > 0)
            return 1;
        if (diff < 0)
            return -1;
        offset++;
    }
    return first[offset] != second[offset];
}

}

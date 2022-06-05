#pragma once

#include "../print.hpp"
#include "../utility.hpp"

#include <iostream>

namespace alien {

class IOErrorStream : public pine::Printer {
public:
    ~IOErrorStream() override = default;

    void print(pine::StringView string) override
    {
        std::cerr << string.data();
    }
};

class IOOutputStream : public pine::Printer {
public:
    ~IOOutputStream() override = default;

    void print(pine::StringView string) override
    {
        std::cout << string.data();
    }
};

template <typename... Args>
inline void print(Args&& ... args)
{
    auto printer = IOOutputStream();
    print_each_with(printer, pine::forward<Args>(args)...);
}

template <typename... Args>
inline void println(Args&& ... args)
{
    print(pine::forward<Args>(args)...);
    print("\n");
}

template <typename... Args>
inline void error(Args&& ... args)
{
    auto printer = IOErrorStream();
    print_each_with(printer, pine::forward<Args>(args)...);
}

template <typename... Args>
inline void errorln(Args&& ... args)
{
    error(pine::forward<Args>(args)..., "\n");
}

};

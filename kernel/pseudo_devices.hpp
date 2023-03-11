#pragma once
#include "file.hpp"

class DevZeroFile : public File {
public:
    inline ssize_t read(char* buf, size_t at_most_bytes) override
    {
        bzero(buf, at_most_bytes);
        return static_cast<ssize_t>(at_most_bytes);    // validated by FileDescription

    }
    inline ssize_t write(char*, size_t bytes) override
    {
        return bytes;
    }
};

class DevNullFile : public File {
public:
    inline ssize_t read(char*, size_t) override
    {
        return 0;
    }
    inline ssize_t write(char*, size_t bytes) override
    {
        return static_cast<ssize_t>(bytes);  // validated by FileDescription
    }
};

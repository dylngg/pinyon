#pragma once
#include "file.hpp"

class DevZeroFile : public File {
public:
    inline size_t read(char* buf, size_t at_most_bytes) override
    {
        bzero(buf, at_most_bytes);
        return at_most_bytes;
    }
    inline size_t write(char*, size_t bytes) override
    {
        return bytes;
    }
};

class DevNullFile : public File {
public:
    inline size_t read(char*, size_t) override
    {
        return 0;
    }
    inline size_t write(char*, size_t bytes) override
    {
        return bytes;
    }
};

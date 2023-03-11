#pragma once
#include <pine/string_view.hpp>
#include <pine/syscall.hpp>

#include "kmalloc.hpp"

class File {
public:
    virtual ~File() = default;
    virtual ssize_t read(char* buf, size_t at_most_bytes) = 0;
    virtual ssize_t write(char* buf, size_t bytes) = 0;
};

class FileDescription {
public:
    ssize_t read(char* buf, size_t at_most_bytes);
    ssize_t write(char* buf, size_t bytes);

private:
    friend class FileTable;

    FileDescription(KOwner<File> file, FileMode mode)
        : m_file(pine::move(file))
        , m_mode(mode)
        , m_ref_count(1) {};

    KOwner<File> m_file;
    FileMode m_mode;
    unsigned m_ref_count;
};

class FileTable {
public:
    FileDescription* open(pine::StringView path, FileMode mode);
    void close(FileDescription&);

private:
    KVector<FileDescription> m_files { kernel_allocator() };
};

FileTable& file_table();

class FileDescriptorTable {
public:
    int open(pine::StringView path, FileMode mode);
    FileDescription* try_get(int fd);
    int close(int fd);
    int dup(int fd);

private:
    int try_insert(FileDescription&);
    KVector<FileDescription*> m_descriptors { kernel_allocator() };
};

#include "device/bcm2835/display.hpp"
#include "file.hpp"
#include "arch/panic.hpp"
#include "pseudo_devices.hpp"
#include "device/pl011/uart.hpp"

#include <pine/limits.hpp>
#include <pine/errno.hpp>

ssize_t FileDescription::read(char *buf, size_t at_most_bytes) {
    if (m_mode == FileMode::Write)
        return -EINVAL;
    if (at_most_bytes > pine::limits<ssize_t>::max)
        return -EINVAL;

    return m_file->read(buf, at_most_bytes);
}

ssize_t FileDescription::write(char *buf, size_t bytes) {
    if (m_mode == FileMode::Read)
        return -EINVAL;
    if (bytes > pine::limits<ssize_t>::max)
        return -EINVAL;

    return m_file->write(buf, bytes);
}

FileDescription* FileTable::open(pine::StringView path, FileMode mode)
{
    pine::Maybe<KOwner<File>> maybe_file;
    if (path == "/dev/null") {
        maybe_file = KOwner<DevNullFile>::try_create(kernel_allocator());
    }
    else if (path == "/dev/zero") {
        maybe_file = KOwner<DevZeroFile>::try_create(kernel_allocator());
    }
    else if (path == "/dev/uart0") {
        maybe_file = KOwner<UARTFile>::try_create(kernel_allocator());
    }
    else if (path == "/dev/display") {
        maybe_file = KOwner<DisplayFile>::try_create(kernel_allocator());
    }

    if (!maybe_file)
        return nullptr;

    if (!m_files.append(FileDescription(*pine::move(maybe_file), mode)))
        return nullptr;

    return &m_files[m_files.length() - 1];
}

void FileTable::close(FileDescription& file_description)
{
    --file_description.m_ref_count;
    if (file_description.m_ref_count == 0) {
        file_description.FileDescription::~FileDescription();
    }
}

int FileDescriptorTable::try_insert(FileDescription& description)
{
    if (m_descriptors.length() > static_cast<size_t>(pine::limits<int>::max))
        return -1;

    int fd_index = 0;
    for (auto& descriptor : m_descriptors) {
        if (descriptor == nullptr) {
            descriptor = &description;
            return fd_index;
        }
        ++fd_index;
    }
    if (!m_descriptors.append(&description))
        return -1;

    return static_cast<int>(m_descriptors.length() - 1);
}

FileTable& file_table()
{
    static FileTable g_file_table {};
    return g_file_table;
}

int FileDescriptorTable::open(pine::StringView path, FileMode mode)
{
    auto* maybe_description = file_table().open(path, mode);
    if (!maybe_description)
        return -1;

    int fd_or_neg = try_insert(*maybe_description);
    if (fd_or_neg == -1) {
        file_table().close(*maybe_description);
        return -1;
    }

    return fd_or_neg;
}

FileDescription *FileDescriptorTable::try_get(int fd) {
    if(fd < 0)
        return nullptr;

    auto fd_index = static_cast<size_t>(fd);
    if (fd_index >= m_descriptors.length())
        return nullptr;

    return m_descriptors[fd_index];
}

int FileDescriptorTable::close(int fd)
{
    if (fd < 0)
        return -EBADF;

    auto fd_index = static_cast<size_t>(fd);
    if (fd_index >= m_descriptors.length())
        return -EBADF;

    auto* descriptor = m_descriptors[fd_index];
    if (!descriptor)
        return -EBADF;

    file_table().close(*descriptor);
    m_descriptors[fd_index] = nullptr;
    return 0;
}

int FileDescriptorTable::dup(int fd)
{
    if (fd < 0)
        return -EBADF;

    auto fd_index = static_cast<size_t>(fd);
    if (fd_index >= m_descriptors.length())
        return -EBADF;

    auto* descriptor = m_descriptors[fd_index];
    if (!descriptor)
        return -EBADF;

    return try_insert(*descriptor);
}

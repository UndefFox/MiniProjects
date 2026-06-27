#pragma once

#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <span>



class FileDescriptor {
    int fd;

public:
    explicit FileDescriptor(const char* path, int flags = O_RDONLY);
    ~FileDescriptor();

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    int get() const noexcept;

    void reset(int new_fd = -1) noexcept;
};

class MemoryMap {
    void* addr;
    size_t size;

public:
    MemoryMap(void* addr, size_t size, int prot, int flags, int fd);
    ~MemoryMap();

    MemoryMap(const MemoryMap&) = delete;
    MemoryMap& operator=(const MemoryMap&) = delete;

    MemoryMap(MemoryMap&& other) noexcept;
    MemoryMap& operator=(MemoryMap&& other) noexcept;

    std::span<std::byte> span() noexcept;

    size_t getSize() const noexcept;

    void reset() noexcept;
};

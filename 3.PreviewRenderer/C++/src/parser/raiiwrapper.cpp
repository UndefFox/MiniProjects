#include "raiiwrapper.h"
#include <stdexcept>
#include <string>
#include <utility>


FileDescriptor::FileDescriptor(const char *path, int flags) {
    fd = ::open(path, flags);
    if (fd == -1) [[unlikely]] {
        throw std::runtime_error(std::string("open() failed for ") + path + ": " + strerror(errno));
    }
}

FileDescriptor::~FileDescriptor() {
    if (fd != -1) {
        ::close(fd);
    }
}

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept : fd(std::exchange(other.fd, -1)) {}

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept {
    reset(std::exchange(other.fd, -1));
    return *this;
}

int FileDescriptor::get() const noexcept { return fd; }

void FileDescriptor::reset(int new_fd) noexcept {
    if (fd != -1) ::close(fd);
    fd = new_fd;
}




MemoryMap::MemoryMap(void* preaddr, size_t size, int prot, int flags, int fd)
    : size(size)
{
    addr = ::mmap(preaddr, size, prot, flags, fd, 0);
    if (addr == MAP_FAILED) [[unlikely]] {
        throw std::runtime_error(std::string("mmap() failed: ") + strerror(errno));
    }
}

MemoryMap::~MemoryMap() {
    if (addr != MAP_FAILED) {
        ::munmap(addr, size);
    }
}

MemoryMap::MemoryMap(MemoryMap &&other) noexcept :
    addr(std::exchange(other.addr, MAP_FAILED)),
    size(std::exchange(other.size, 0))
{}

MemoryMap &MemoryMap::operator=(MemoryMap &&other) noexcept {
    reset();
    addr = std::exchange(other.addr, MAP_FAILED);
    size = std::exchange(other.size, 0);
    return *this;
}

std::span<std::byte> MemoryMap::span() noexcept {
    return std::span<std::byte>(static_cast<std::byte*>(addr), size);
}

size_t MemoryMap::getSize() const noexcept {
    return size;
}

void MemoryMap::reset() noexcept {
    if (addr != MAP_FAILED) {
        ::munmap(addr, size);
        addr = MAP_FAILED;
        size = 0;
    }
}

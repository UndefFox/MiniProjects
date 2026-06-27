#pragma once

#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raiiwrapper.h"



namespace Parser::Binary {

template <typename T>
decltype(T::result) read(const std::string& filePath) {
    FileDescriptor fd(filePath.c_str(), O_RDONLY);

    struct stat sb;
    if (::fstat(fd.get(), &sb) < 0) throw std::runtime_error("fstat failed");

    MemoryMap mapped(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd.get());

    T adapter{};
    adapter.process(mapped.span());

    return adapter.result;
}

}
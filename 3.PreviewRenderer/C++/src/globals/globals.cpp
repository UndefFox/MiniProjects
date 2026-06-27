#include "globals.h"

#include <filesystem>



RootDirectory rootDirectory{};

RootDirectory::RootDirectory() :
    path(std::filesystem::canonical("/proc/self/exe").parent_path().parent_path().string())
{ }

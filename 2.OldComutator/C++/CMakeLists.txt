cmake_minimum_required(VERSION 3.24.1)

project(OldComutator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


if (MSVC)
    add_compile_options(/arch:AVX2)
else ()
    add_compile_options(-mavx2)
endif ()
#add_compile_options(-O3)


add_executable(oldcom)
target_sources(oldcom
    PRIVATE
        src/main.cpp
        src/backend.h src/backend.cpp
        src/eventpipe.h src/eventpipe.cpp
        src/parameters.h src/parameters.cpp
        src/diffiehellman.h src/diffiehellman.cpp

        src/network/socket.h src/network/socket.cpp
        src/network/foxtunnel.h src/network/foxtunnel.cpp
        src/network/foxpackage.h src/network/foxpackage.cpp
)
target_include_directories(oldcom
    PRIVATE
        src/
)

install(TARGETS oldcom
    RUNTIME DESTINATION "oldcom"
)

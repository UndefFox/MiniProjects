cmake_minimum_required(VERSION 3.24.1)

project(NetMesh VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


# ----- Packages required -----

find_package(OpenSSL REQUIRED)



# ----- Sources -----

add_library(utils STATIC)
target_sources(utils
PRIVATE
    src/utils/signalmaskguard.h src/utils/signalmaskguard.cpp
    src/utils/terminalguard.h src/utils/terminalguard.cpp
    src/utils/signaldesc.h src/utils/signaldesc.cpp
    src/utils/socket.h src/utils/socket.cpp
    src/utils/pipe.h src/utils/pipe.cpp
)
target_include_directories(utils
PRIVATE
    src/utils
PUBLIC
    src
)

add_library(server STATIC)
target_sources(server
PRIVATE
    src/server/serverstate.h src/server/serverstate.cpp
    src/server/protocols.h src/server/protocols.cpp
    src/server/enctryptor.h src/server/enctryptor.cpp
)
target_include_directories(server
PRIVATE
    src/server
PUBLIC
    src
)
target_link_libraries(server
PRIVATE
    OpenSSL::Crypto
)

add_library(tui STATIC)
target_sources(tui
PRIVATE
    src/tui/textbox.h src/tui/textbox.cpp
    src/tui/widget.h src/tui/widget.cpp
    src/tui/tui.h src/tui/tui.cpp
    src/tui/rectangle.hpp
    src/tui/sequences.h
)
target_include_directories(tui
PRIVATE
    src/tui
PUBLIC
    src
)

add_executable(netmesh)
target_sources(netmesh
PRIVATE
    src/workers/worker.h src/workers/worker.cpp
    src/workers/threadssharedstate.hpp
    src/workers/tuiWorker.hpp
    src/workers/packageprocessorworker.hpp
    src/workers/registrationworker.hpp
    src/params.h src/params.cpp
    src/app.h src/app.cpp
    src/generalerror.h
    src/main.cpp
)
target_include_directories(netmesh
PRIVATE
    src
)
target_link_libraries(netmesh
PRIVATE
    utils
    server
    tui
)
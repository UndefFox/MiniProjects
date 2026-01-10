cmake_minimum_required(VERSION 3.24.1)

project(SnowFall VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_executable(snowfall)
target_sources(snowfall
    PRIVATE
        src/main.cpp
        src/signalwatcher.cpp src/signalwatcher.h
        src/forwardlist.hpp
        src/particleengine.cpp src/particleengine.h
)

install(TARGETS snowfall
    RUNTIME DESTINATION "Snowfall"
)

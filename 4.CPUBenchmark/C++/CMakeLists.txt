cmake_minimum_required(VERSION 3.24.1)

project(CPUBenchmark VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


# ----- Packages -----

find_package(Catch2 3)
find_package(BLAS)


# ----- Sources -----

add_library(squarematrix STATIC)
target_sources(squarematrix
PRIVATE
    src/squarematrix.h src/squarematrix.cpp
    src/alignedallocator.hpp
)
target_include_directories(squarematrix
PUBLIC
    src/
)
target_compile_options(squarematrix PRIVATE
    -O3
    -march=native
    -fno-math-errno
    -ffp-contract=fast
    -mavx2
    -mfma
)

if (BLAS_FOUND)
    target_link_libraries(squarematrix
    PRIVATE
        BLAS::BLAS
    )
endif()

add_executable(foxbench)
target_sources(foxbench
PRIVATE
    src/main.cpp
    src/params.h src/params.cpp
    src/benchmark.h src/benchmark.cpp
    src/log.h src/log.cpp
    src/generalerror.h
)
target_include_directories(foxbench
PRIVATE
    src/
)
target_link_libraries(foxbench
PRIVATE
    squarematrix
)

install(TARGETS foxbench)


# ----- Tests -----

if (Catch2_FOUND)
    add_executable(tests)
    target_sources(tests
        PRIVATE
            tests/squarematrix.cpp
    )
    target_link_libraries(tests
        PRIVATE
            Catch2::Catch2WithMain
            squarematrix
    )
    add_test(NAME GlobalTest COMMAND tests)
endif()

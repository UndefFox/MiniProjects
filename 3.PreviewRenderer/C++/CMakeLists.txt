cmake_minimum_required(VERSION 3.24.1)

project(PreviewRenderer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


# ----- Packages required -----

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_package(glm REQUIRED)

include(FetchContent)

FetchContent_Declare(
  trex
  GIT_REPOSITORY https://github.com/KyrietS/trex.git
  GIT_TAG 6438bd7
  GIT_SHALLOW 1
)
FetchContent_MakeAvailable(trex)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG 31c1ad3
  GIT_SHALLOW 1
  SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/stb-src
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb
    INTERFACE
        ${CMAKE_BINARY_DIR}/_deps/stb-src
)


# ----- Additional commands -----

function(compile_shaders target_name)
    foreach(shader ${ARGN})
        get_filename_component(name ${shader} NAME)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${target_name}.dir/${shader}.spv")

        add_custom_command(OUTPUT ${output}
            COMMAND Vulkan::glslc "${CMAKE_CURRENT_SOURCE_DIR}/${shader}" -o ${output}
            DEPENDS ${shader})
        list(APPEND outputs ${output})
    endforeach()

    add_custom_target(${target_name} DEPENDS ${outputs} SOURCES ${ARGN})
    install(FILES ${outputs} DESTINATION "data/shaders")
endfunction()



# ----- Sources -----

add_library(globals)
target_sources(globals
    PRIVATE
        src/globals/globals.cpp src/globals/globals.h
)
target_include_directories(globals
    PUBLIC
        src
)

add_library(parser)
target_sources(parser
    PRIVATE
        src/parser/raiiwrapper.cpp src/parser/raiiwrapper.h
        src/parser/binary.hpp
        src/parser/obj.hpp
)
target_include_directories(parser
    PUBLIC
        src
)

compile_shaders(shaders
    src/renderer/shaders/triangle.vert
    src/renderer/shaders/triangle.frag

    src/renderer/shaders/line.vert
    src/renderer/shaders/line.frag
)

add_library(renderer)
target_sources(renderer
    PRIVATE
        src/renderer/renderpass.cpp src/renderer/renderpass.h
        src/renderer/renderer.cpp src/renderer/renderer.h
        src/renderer/storage.cpp src/renderer/storage.h
        src/renderer/device.cpp src/renderer/device.h
        src/renderer/types.cpp src/renderer/types.h
        src/renderer/renderparams.h
)
target_link_libraries(renderer
    PRIVATE
        parser
        globals
)
add_dependencies(renderer
    shaders
)
target_include_directories(renderer
    PUBLIC
        src
)

add_executable(preivewrenderer)
target_sources(preivewrenderer
    PRIVATE
        src/main.cpp
        src/geometrygenerator.cpp src/geometrygenerator.h
        src/objAdapter.cpp src/objAdapter.h
        src/params.cpp src/params.h
        src/image.cpp src/image.h
)
target_link_libraries(preivewrenderer
    PRIVATE
        renderer
        trex
        stb
        Vulkan::Vulkan
        glm::glm
)


install(TARGETS preivewrenderer)

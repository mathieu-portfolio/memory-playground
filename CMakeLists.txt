cmake_minimum_required(VERSION 3.20)

project(memory-playground
    VERSION 0.1.0
    DESCRIPTION "Educational CPU memory hierarchy visualization"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG QUIET)
if (NOT raylib_FOUND)
    find_package(raylib QUIET)
endif()

if (NOT raylib_FOUND)
    message(FATAL_ERROR
        "raylib was not found. Install raylib first, for example with vcpkg:\n"
        "  vcpkg install raylib\n"
        "Then configure with your vcpkg toolchain file."
    )
endif()

add_executable(memory-playground
    src/main.cpp
)

target_link_libraries(memory-playground PRIVATE raylib)

if (MSVC)
    target_compile_options(memory-playground PRIVATE /W4 /permissive-)
else()
    target_compile_options(memory-playground PRIVATE -Wall -Wextra -Wpedantic)
endif()

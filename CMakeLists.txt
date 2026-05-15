cmake_minimum_required(VERSION 3.20)

project(memory-playground
    VERSION 0.1.0
    DESCRIPTION "Educational CPU memory hierarchy visualization"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MEMORY_PLAYGROUND_BUILD_APP "Build the raylib visualization app" ON)
option(MEMORY_PLAYGROUND_BUILD_TESTS "Build unit tests" ON)

add_library(memory-playground-sim INTERFACE)
target_include_directories(memory-playground-sim INTERFACE src)

if (MEMORY_PLAYGROUND_BUILD_APP)
    find_package(raylib CONFIG QUIET)
    if (NOT raylib_FOUND)
        find_package(raylib QUIET)
    endif()

    if (raylib_FOUND)
        add_executable(memory-playground
            src/main.cpp
        )

        target_link_libraries(memory-playground PRIVATE memory-playground-sim raylib)

        if (MSVC)
            target_compile_options(memory-playground PRIVATE /W4 /permissive-)
        else()
            target_compile_options(memory-playground PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    else()
        message(WARNING
            "raylib was not found, so the visualization app target will not be built.\n"
            "Install raylib, for example with vcpkg:\n"
            "  vcpkg install raylib\n"
            "Then configure with your vcpkg toolchain file."
        )
    endif()
endif()

if (MEMORY_PLAYGROUND_BUILD_TESTS)
    include(CTest)

    add_executable(memory-playground-tests
        tests/simulation_tests.cpp
    )

    target_link_libraries(memory-playground-tests PRIVATE memory-playground-sim)

    if (MSVC)
        target_compile_options(memory-playground-tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(memory-playground-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME memory-playground-tests COMMAND memory-playground-tests)
endif()

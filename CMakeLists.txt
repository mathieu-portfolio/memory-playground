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

    if (NOT raylib_FOUND)
        message(FATAL_ERROR
            "raylib was not found, so the visualization app cannot be built.\n"
            "Install raylib, for example with vcpkg:\n"
            "  vcpkg install raylib\n"
            "Then configure with:\n"
            "  cmake --preset app-vcpkg-debug\n"
            "Tests can still be built without raylib:\n"
            "  cmake --preset tests-debug"
        )
    endif()

    add_executable(memory-playground
        src/input.cpp
        src/main.cpp
        src/renderer.cpp
        src/rendering/renderer_memory.cpp
        src/rendering/renderer_dashboard.cpp
    )

    target_link_libraries(memory-playground PRIVATE memory-playground-sim raylib)

    if (MSVC)
        target_compile_options(memory-playground PRIVATE /W4 /permissive-)
    else()
        target_compile_options(memory-playground PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if (MEMORY_PLAYGROUND_BUILD_TESTS)
    include(CTest)
    include(GoogleTest)

    find_package(GTest CONFIG QUIET)
    if (NOT GTest_FOUND)
        find_package(GTest QUIET)
    endif()

    if (NOT GTest_FOUND)
        message(FATAL_ERROR
            "GoogleTest was not found, so tests cannot be built.\n"
            "Install it, for example with vcpkg:\n"
            "  vcpkg install gtest\n"
            "Then configure tests with your vcpkg toolchain file or use:\n"
            "  cmake --preset tests-vcpkg-debug"
        )
    endif()

    add_executable(memory-playground-tests
        tests/simulation_tests.cpp
    )

    target_link_libraries(memory-playground-tests PRIVATE memory-playground-sim GTest::gtest_main)

    if (MSVC)
        target_compile_options(memory-playground-tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(memory-playground-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    gtest_discover_tests(memory-playground-tests)
endif()

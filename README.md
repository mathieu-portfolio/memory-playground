# memory-playground

`memory-playground` is a C++20/raylib educational sandbox for exploring how data moves through a simplified CPU memory hierarchy.

It is not a CPU emulator. The goal is to make cache behavior visible and interactive enough that arrays, random access, and linked-list pointer chasing feel different immediately.

## Phase 1 Features

- RAM cells grouped into configurable cache lines
- configurable L1 cache capacity
- visual registers
- sequential, random, and linked-list traversal modes
- animated data flow for hits and misses
- access timeline showing recent hits, misses, current access, and evictions
- live hit-rate graph
- mouse sliders and keyboard-adjustable experiment settings
- challenge panel with active, completed, and failed objectives
- learning panel that explains the current access pattern and last access

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- raylib for the interactive app
- GoogleTest for tests
- Bash for helper scripts

The tests do not require raylib, but they do use GoogleTest.

## Run The App

With raylib available to CMake:

```bash
./scripts/run.sh
```

With vcpkg:

```bash
vcpkg install raylib gtest
export VCPKG_ROOT=/path/to/vcpkg
PRESET=app-vcpkg-debug ./scripts/run.sh
```

Manual equivalent:

```bash
cmake --preset app-debug
cmake --build --preset app-debug
./build/app-debug/memory-playground
```

On Visual Studio generators, the executable is usually under `build/app-debug/Debug/`.

## Run Tests

```bash
./scripts/tests.sh
```

With vcpkg:

```bash
export VCPKG_ROOT=/path/to/vcpkg
PRESET=tests-vcpkg-debug ./scripts/tests.sh
```

Manual equivalent:

```bash
cmake --preset tests-debug
cmake --build --preset tests-debug
ctest --preset tests-debug
```

## Controls

| Key | Action |
| --- | --- |
| Space | Pause or resume |
| R | Reset current simulation |
| 1 | Sequential array traversal |
| 2 | Random access traversal |
| 3 | Linked-list traversal |
| Up / Down | Increase / decrease simulation speed |
| [ / ] | Decrease / increase cache line size |
| - / = | Decrease / increase L1 cache capacity |
| H / J | Decrease / increase cache hit latency |
| N / M | Decrease / increase cache miss latency |
| Enter | Step once while paused |

You can also drag the sliders in the Experiment panel. Changing cache line size, cache capacity, hit latency, or miss latency resets the simulation cleanly so comparisons remain deterministic.

The window is resizable. If panels feel cramped, expand the window; the RAM area and side panels adapt to the available space.

## Challenge System

The challenge panel tracks small live objectives:

- reach more than 95% hit rate
- keep average access cost under 10 cycles
- load 10 values into registers in a row
- complete 64 accesses with fewer than 12 misses

Each challenge displays a target, current progress, and state:

- Active: still in progress
- Complete: objective completed
- Failed: objective can no longer be completed in the current run

The challenge logic is intentionally simple and deterministic. It is meant to guide experiments, not judge real CPU performance.

## Simplified Timing Model

The simulator uses configurable educational cycle costs:

- cache hit: default 4 cycles
- cache miss: default 100 cycles

These values are deliberately simplified. A miss means the requested cache line was not in L1, so the simulator charges the miss latency and loads the full line into L1. A hit means the line was already present, so the simulator charges the hit latency.

The average access cost is:

```text
total estimated cycles / total accesses
```

## Project Layout

```text
.
+-- CMakeLists.txt
+-- CMakePresets.json
+-- README.md
+-- scripts/
|   +-- build.sh
|   +-- run.sh
|   +-- tests.sh
+-- src/
|   +-- app_config.hpp
|   +-- input.cpp
|   +-- input.hpp
|   +-- main.cpp
|   +-- renderer.cpp
|   +-- renderer.hpp
|   +-- simulation.hpp
|   +-- simulation/
|       +-- access_pattern.hpp
|       +-- cache.hpp
|       +-- challenge.hpp
|       +-- constants.hpp
|       +-- experiment_settings.hpp
|       +-- history.hpp
|       +-- memory.hpp
|       +-- metrics.hpp
|       +-- register_file.hpp
|       +-- simulation_state.hpp
+-- tests/
    +-- simulation_tests.cpp
```

Key pieces:

- `src/simulation/` contains the raylib-free simulator.
- `src/simulation.hpp` is the simulator facade used by app and tests.
- `src/input.cpp` contains keyboard controls.
- `src/renderer.cpp` contains raylib drawing and layout.
- `tests/simulation_tests.cpp` covers simulator behavior.

## CMake Presets

| Preset | Build directory | Purpose |
| --- | --- | --- |
| `app-debug` | `build/app-debug` | Build the raylib app |
| `app-vcpkg-debug` | `build/app-vcpkg-debug` | Build the raylib app using `$VCPKG_ROOT` |
| `tests-debug` | `build/tests-debug` | Build GoogleTest tests only, without raylib |
| `tests-vcpkg-debug` | `build/tests-vcpkg-debug` | Build GoogleTest tests using `$VCPKG_ROOT` |

If raylib is not found while configuring an app preset, CMake fails with an explicit message. The test presets do not require raylib.

## Roadmap

Phase 2:

- heatmap of frequently accessed memory
- manual mode for choosing addresses
- stride traversal mode
- tooltips for cache lines, metrics, and challenges

Phase 3:

- Structure of Arrays vs Array of Structures
- prefetching
- false sharing
- SIMD lanes

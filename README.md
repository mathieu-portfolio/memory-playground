# memory-playground

`memory-playground` is a C++20/raylib educational visualization for exploring how data moves through a simplified CPU memory hierarchy.

It shows RAM cells grouped into cache lines, an L1 cache, a visual register file, cache hits and misses, evictions, sequential traversal, random access, linked-list pointer chasing, and live performance metrics. This is not a CPU emulator; it favors clarity over hardware realism.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- raylib for the interactive visualization app
- Bash for the helper scripts

The tests do not require raylib.

## Run The App

With raylib available to CMake:

```bash
./scripts/run.sh
```

With vcpkg:

```bash
vcpkg install raylib
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
| R | Reset the current simulation |
| 1 | Sequential array traversal |
| 2 | Random access traversal |
| 3 | Linked-list traversal |
| Up | Increase simulation speed |
| Down | Decrease simulation speed |
| Tab | Toggle controls/help overlay |
| Enter | Step once while paused |

The window is resizable. If panels feel cramped, expand the window; the RAM area and side panels adapt to the available space.

## Project Layout

```text
.
+-- CMakeLists.txt
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
|       +-- constants.hpp
|       +-- memory.hpp
|       +-- metrics.hpp
|       +-- register_file.hpp
|       +-- simulation_state.hpp
+-- tests/
    +-- simulation_tests.cpp
```

Key pieces:

- `src/simulation.hpp` is the public facade for the raylib-free simulator.
- `src/simulation/` contains the simulator pieces.
- `src/renderer.cpp` contains raylib drawing code and responsive layout.
- `src/input.cpp` contains keyboard handling.
- `src/main.cpp` contains startup and the app loop.
- `tests/simulation_tests.cpp` contains unit tests for the simulator.
- `CMakePresets.json` defines isolated app and test build directories.
- `scripts/build.sh`, `scripts/run.sh`, and `scripts/tests.sh` provide common workflows.

## Simplified Model

- RAM has 128 cells.
- A cache line contains 8 cells.
- L1 cache contains 8 cache lines.
- Cache replacement uses FIFO.
- A cache hit costs 4 estimated cycles.
- A cache miss costs 100 estimated cycles.
- Registers are visual slots, not real CPU registers.

Each memory access follows this conceptual path:

```text
RAM -> L1 Cache -> Register
```

On a miss, the whole cache line containing the requested address is loaded into L1. On a hit, the line is already present in L1.

## CMake Presets

| Preset | Build directory | Purpose |
| --- | --- | --- |
| `app-debug` | `build/app-debug` | Build the raylib app and tests |
| `app-vcpkg-debug` | `build/app-vcpkg-debug` | Build the raylib app using `$VCPKG_ROOT` |
| `tests-debug` | `build/tests-debug` | Build tests only, without raylib |

If raylib is not found while configuring an app preset, CMake fails with an explicit message. The `tests-debug` preset does not require raylib.

## Future Ideas

- Structure of Arrays vs Array of Structures
- prefetching
- false sharing
- branch prediction
- SIMD lanes
- data-oriented design and ECS-style layouts
- configurable cache sizes and replacement policies
- guided lesson scenarios

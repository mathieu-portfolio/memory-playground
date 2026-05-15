# memory-playground

`memory-playground` is a C++20/raylib educational visualization for exploring how data moves through a simplified CPU memory hierarchy.

It shows:

- RAM as addressable cells grouped into cache lines
- an L1 cache with fixed-size cache lines
- a small visual register file
- cache hits, misses, and evictions
- sequential array traversal, random access, and linked-list pointer chasing
- live metrics for hits, misses, hit rate, and estimated cycles

This is not a CPU emulator. The project intentionally favors clarity over hardware realism so that memory access patterns are easy to see and reason about.

## Why This Exists

Modern CPUs are fast, but memory access patterns can dominate performance. This project makes that invisible behavior visible.

Sequential array traversal demonstrates spatial locality: one cache miss loads a whole cache line, and nearby accesses can then hit.

Random access demonstrates poor locality: jumps across RAM often miss because the needed line is not already cached.

Linked-list traversal demonstrates pointer chasing: each node stores the next address, so traversal jumps through RAM instead of walking contiguous memory.

## Requirements

- CMake 3.20 or newer
- A C++20 compiler
- raylib for the interactive visualization app
- Bash for the helper scripts in `scripts/`

The tests do not require raylib.

## Run The App

With vcpkg:

```bash
vcpkg install raylib
export VCPKG_ROOT=/path/to/vcpkg
PRESET=app-vcpkg-debug ./scripts/run.sh
```

With raylib already available to CMake:

```bash
./scripts/run.sh
```

The script configures the `app-debug` CMake preset, builds it, and launches `memory-playground`.

Equivalent manual commands:

```bash
cmake --preset app-debug
cmake --build --preset app-debug
./build/app-debug/memory-playground
```

On Visual Studio generators, the executable is usually under `build/app-debug/Debug/`.

## Run Tests

Tests exercise the simulator logic without starting raylib:

```bash
./scripts/test.sh
```

Equivalent manual commands:

```bash
cmake --preset tests-debug
cmake --build --preset tests-debug
ctest --preset tests-debug
```

The test suite currently covers:

- sequential traversal producing one miss followed by cache hits within a line
- FIFO cache eviction
- register slot rotation
- linked-list traversal visiting every memory cell

## Build Only

```bash
./scripts/build.sh
```

By default, this builds the `app-debug` preset. To use another preset:

```bash
PRESET=app-vcpkg-debug ./scripts/build.sh
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

## Project Layout

```text
.
├── CMakeLists.txt
├── README.md
├── scripts/
│   ├── build.sh
│   ├── run.sh
│   ├── test.sh
│   └── tests.sh
├── src/
│   ├── main.cpp
│   └── simulation.hpp
└── tests/
    └── simulation_tests.cpp
```

Key pieces:

- `src/simulation.hpp` contains the raylib-free simulator.
- `src/main.cpp` contains rendering, input handling, and the app loop.
- `tests/simulation_tests.cpp` contains unit tests for the simulator.
- `CMakePresets.json` defines isolated app and test build directories.
- `scripts/build.sh`, `scripts/run.sh`, `scripts/test.sh`, and `scripts/tests.sh` provide common workflows.

## Simplified Model

The hardware model is deliberately small and pedagogical:

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

## CMake Options

| Option | Default | Description |
| --- | --- | --- |
| `MEMORY_PLAYGROUND_BUILD_APP` | `ON` | Build the raylib visualization app |
| `MEMORY_PLAYGROUND_BUILD_TESTS` | `ON` | Build the simulator test target |

Useful presets:

| Preset | Build directory | Purpose |
| --- | --- | --- |
| `app-debug` | `build/app-debug` | Build the raylib app and tests |
| `app-vcpkg-debug` | `build/app-vcpkg-debug` | Build the raylib app using `$VCPKG_ROOT` |
| `tests-debug` | `build/tests-debug` | Build tests only, without raylib |

If raylib is not found while configuring an app preset, CMake fails with an explicit message. The `tests-debug` preset does not require raylib.

## Future Ideas

This first version is intentionally small. Possible future modules include:

- Structure of Arrays vs Array of Structures
- prefetching
- false sharing
- branch prediction
- SIMD lanes
- data-oriented design and ECS-style layouts
- configurable cache sizes and replacement policies
- guided lesson scenarios

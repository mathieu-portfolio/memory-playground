# memory-playground

`memory-playground` is a small C++20/raylib educational visualization that makes the CPU memory hierarchy visible. It is not a CPU emulator. It is a simplified simulator for building intuition about how data moves from RAM into cache and then into registers.

The first version focuses on:

- RAM cells grouped into cache lines
- a tiny L1 cache
- a pedagogical register file
- sequential, random, and linked-list traversal patterns
- live cache hit/miss metrics
- simplified cycle estimates

## Educational Goals

The project is designed to explain why memory layout matters.

Sequential array traversal walks contiguous addresses. After one miss loads a cache line, the next nearby accesses tend to hit because the data is already in L1.

Random traversal jumps around memory, so it often lands in cache lines that are not loaded.

Linked-list traversal follows `next` pointers stored in each cell. The nodes are intentionally shuffled through RAM, so the access pattern demonstrates why pointer chasing is usually cache-unfriendly.

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

## Build

Requirements:

- CMake 3.20+
- A C++20 compiler
- raylib

Example with vcpkg:

```powershell
vcpkg install raylib
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
.\build\Debug\memory-playground.exe
```

For a non-vcpkg raylib install, make sure CMake can find raylib through your normal package search paths:

```powershell
cmake -S . -B build
cmake --build build
```

## Architecture

The first version keeps the implementation intentionally small:

- `MemoryCell` represents one simplified RAM address.
- `SimpleCache` models an 8-line L1 cache.
- `RegisterFile` shows recently processed values.
- `AccessPattern` defines traversal behavior.
- `SequentialPattern`, `RandomPattern`, and `LinkedListPattern` produce addresses.
- `SimulationState` owns memory, cache, registers, metrics, and current traversal state.
- `Renderer` draws the hierarchy, highlights, flow animation, metrics, and controls.

The code favors explicit logic over general frameworks so that the educational model is easy to inspect and modify.

## Simplified Hardware Assumptions

These assumptions are deliberately unrealistic but useful for teaching:

- RAM has 64 cells.
- A cache line contains 8 cells.
- L1 cache contains 8 cache lines.
- Cache replacement uses FIFO.
- A cache hit costs 4 estimated cycles.
- A cache miss costs 100 estimated cycles.
- Registers are visual slots, not real CPU registers.
- No real instruction decoding, hardware counters, threads, prefetching, branch prediction, or SIMD execution are modeled.

When an address is accessed, the simulator shows the conceptual data path:

```text
RAM -> L1 Cache -> Register
```

On a cache hit, the data is already represented in L1. On a miss, the entire cache line containing the address is loaded into L1 first.

## Future Extension Ideas

The project is structured so future modules can be added without turning this first version into a full emulator:

- Structure of Arrays vs Array of Structures
- prefetching
- false sharing
- branch prediction
- SIMD lanes
- data-oriented design and ECS-style layouts
- configurable cache sizes and replacement policies
- scripted lessons or guided scenarios

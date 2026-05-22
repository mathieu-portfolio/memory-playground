# Architecture

## Goals

The project is designed as a systems exploration platform focused on:

- cache locality
- access pattern analysis
- instrumentation
- deterministic simulation
- interactive visualization

## Core Systems

### SimulationState

Owns the active simulation state and memory hierarchy.

### MetricsCollector

Aggregates runtime metrics:

- cache hits/misses
- evictions
- access frequency
- simulated cycles

### TraceEvent

Structured event pipeline used for:

- timeline visualization
- replay analysis
- offline export
- debugging

### BenchmarkRunner

Executes deterministic benchmark scenarios and compares metrics across runs.

## Design Principles

### Simulation / Rendering Separation

Simulation logic is isolated from rendering code to keep the project:

- deterministic
- testable
- extensible
- benchmark-friendly

### Deterministic Execution

Scenarios use fixed seeds and reproducible access generation.

This allows:
- comparable benchmark runs
- stable instrumentation
- replay-style analysis

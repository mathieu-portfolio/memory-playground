#pragma once

#include "simulation/access_pattern.hpp"
#include "simulation/cache.hpp"
#include "simulation/metrics.hpp"
#include "simulation/register_file.hpp"

#include <algorithm>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

namespace memory_playground
{
struct AccessEvent
{
    int address = 0;
    int lineStart = 0;
    int cacheSlot = 0;
    int registerSlot = 0;
    bool hit = false;
    std::optional<int> evictedLineStart;
    float age = 0.0f;
};

class SimulationState
{
public:
    SimulationState()
    {
        memory.reserve(kRamCellCount);
        for (int i = 0; i < kRamCellCount; ++i)
        {
            memory.push_back(MemoryCell{i, 1000 + i, 0});
        }
        buildLinkedList();
        setPattern(PatternKind::Sequential);
    }

    void setPattern(PatternKind kind)
    {
        patternKind = kind;
        switch (patternKind)
        {
        case PatternKind::Sequential:
            pattern = std::make_unique<SequentialPattern>();
            break;
        case PatternKind::Random:
            pattern = std::make_unique<RandomPattern>();
            break;
        case PatternKind::LinkedList:
            pattern = std::make_unique<LinkedListPattern>();
            break;
        }
        reset();
    }

    void reset()
    {
        cache.reset();
        registers.reset();
        metrics = Metrics{};
        tick = 0;
        stepAccumulator = 0.0f;
        lastEvent.reset();
        pattern->reset(memory);
    }

    void update(float dt)
    {
        cache.update(dt);
        registers.update(dt);
        if (lastEvent)
        {
            lastEvent->age += dt;
            if (lastEvent->age > 1.0f)
            {
                lastEvent.reset();
            }
        }

        if (paused)
        {
            return;
        }

        stepAccumulator += dt * accessesPerSecond;
        while (stepAccumulator >= 1.0f)
        {
            performAccess();
            stepAccumulator -= 1.0f;
        }
    }

    void stepOnce()
    {
        performAccess();
    }

    void faster()
    {
        accessesPerSecond = std::min(12.0f, accessesPerSecond + 0.5f);
    }

    void slower()
    {
        accessesPerSecond = std::max(0.5f, accessesPerSecond - 0.5f);
    }

    const std::vector<MemoryCell>& getMemory() const { return memory; }
    const SimpleCache& getCache() const { return cache; }
    const RegisterFile& getRegisters() const { return registers; }
    const Metrics& getMetrics() const { return metrics; }
    const AccessPattern& getPattern() const { return *pattern; }
    const std::optional<AccessEvent>& getLastEvent() const { return lastEvent; }
    float speed() const { return accessesPerSecond; }

    bool paused = false;
    bool showOverlay = true;

private:
    void performAccess()
    {
        const int address = pattern->nextAddress(memory);
        const CacheAccess cacheAccess = cache.access(address, tick++);
        const int registerSlot = registers.write(address, memory[address].value);

        metrics.totalAccesses += 1;
        if (cacheAccess.hit)
        {
            metrics.cacheHits += 1;
            metrics.estimatedCycles += kHitCycles;
        }
        else
        {
            metrics.cacheMisses += 1;
            metrics.estimatedCycles += kMissCycles;
        }

        lastEvent = AccessEvent{
            address,
            cacheAccess.lineStart,
            cacheAccess.slotIndex,
            registerSlot,
            cacheAccess.hit,
            cacheAccess.evictedLineStart,
            0.0f
        };
    }

    void buildLinkedList()
    {
        std::vector<int> addresses(kRamCellCount);
        std::iota(addresses.begin(), addresses.end(), 0);
        std::mt19937 generator{42};
        std::shuffle(addresses.begin(), addresses.end(), generator);

        for (int i = 0; i < kRamCellCount; ++i)
        {
            const int address = addresses[i];
            const int next = addresses[(i + 1) % kRamCellCount];
            memory[address].nextAddress = next;
        }
    }

    std::vector<MemoryCell> memory;
    SimpleCache cache;
    RegisterFile registers;
    Metrics metrics;
    std::unique_ptr<AccessPattern> pattern;
    PatternKind patternKind = PatternKind::Sequential;
    std::optional<AccessEvent> lastEvent;
    float accessesPerSecond = 2.0f;
    float stepAccumulator = 0.0f;
    int tick = 0;
};
}

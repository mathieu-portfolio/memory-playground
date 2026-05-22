#pragma once

#include "simulation/access_pattern.hpp"
#include "simulation/cache.hpp"
#include "simulation/challenge.hpp"
#include "simulation/experiment_settings.hpp"
#include "simulation/history.hpp"
#include "simulation/metrics.hpp"
#include "simulation/instrumentation.hpp"
#include "simulation/simulation_run.hpp"
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
    int cycles = 0;
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
        clampSettings(settings);
        cache.configure(settings.cacheLineSize, settings.cacheLineCount);
        cache.reset();
        registers.reset();
        metrics = Metrics{};
        tick = 0;
        stepAccumulator = 0.0f;
        consecutiveRegisterLoads = 0;
        lastEvent.reset();
        accessHistory.clear();
        performanceHistory.clear();
        challenges.reset();
        challenges.update(0, 0, 0.0f, 0.0f, 0);
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

        stepAccumulator += dt * settings.accessesPerSecond;
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
        setSpeed(settings.accessesPerSecond + 0.5f);
    }

    void slower()
    {
        setSpeed(settings.accessesPerSecond - 0.5f);
    }

    void adjustCacheLineSize(int delta)
    {
        setCacheLineSize(settings.cacheLineSize + delta);
    }

    void adjustCacheLineCount(int delta)
    {
        setCacheLineCount(settings.cacheLineCount + delta);
    }

    void adjustHitCycles(int delta)
    {
        setHitCycles(settings.hitCycles + delta);
    }

    void adjustMissCycles(int delta)
    {
        setMissCycles(settings.missCycles + delta);
    }

    void setSpeed(float value)
    {
        settings.accessesPerSecond = std::clamp(value, 0.5f, 12.0f);
    }

    void setCacheLineSize(int value)
    {
        const int oldValue = settings.cacheLineSize;
        settings.cacheLineSize = value;
        clampSettings(settings);
        if (settings.cacheLineSize != oldValue)
        {
            reset();
        }
    }

    void setCacheLineCount(int value)
    {
        const int oldValue = settings.cacheLineCount;
        settings.cacheLineCount = value;
        clampSettings(settings);
        if (settings.cacheLineCount != oldValue)
        {
            reset();
        }
    }

    void setHitCycles(int value)
    {
        const int oldValue = settings.hitCycles;
        settings.hitCycles = value;
        clampSettings(settings);
        if (settings.hitCycles != oldValue)
        {
            reset();
        }
    }

    void setMissCycles(int value)
    {
        const int oldValue = settings.missCycles;
        settings.missCycles = value;
        clampSettings(settings);
        if (settings.missCycles != oldValue)
        {
            reset();
        }
    }

    void applySettings(const ExperimentSettings& nextSettings)
    {
        const ExperimentSettings oldSettings = settings;
        settings = nextSettings;
        clampSettings(settings);
        if (settings.cacheLineSize != oldSettings.cacheLineSize ||
            settings.cacheLineCount != oldSettings.cacheLineCount ||
            settings.hitCycles != oldSettings.hitCycles ||
            settings.missCycles != oldSettings.missCycles)
        {
            reset();
        }
    }

    void resetExperimentSettings()
    {
        settings = ExperimentSettings{};
        reset();
    }

    const std::vector<MemoryCell>& getMemory() const { return memory; }
    const SimpleCache& getCache() const { return cache; }
    const RegisterFile& getRegisters() const { return registers; }
    const Metrics& getMetrics() const { return metrics; }
    const AccessPattern& getPattern() const { return *pattern; }
    const std::optional<AccessEvent>& getLastEvent() const { return lastEvent; }
    const ExperimentSettings& getSettings() const { return settings; }
    const AccessHistory& getAccessHistory() const { return accessHistory; }
    const PerformanceHistory& getPerformanceHistory() const { return performanceHistory; }
    const ChallengeSystem& getChallenges() const { return challenges; }
    const MetricsCollector& getInstrumentationMetrics() const { return metricsCollector; }
    const SimulationRun& getSimulationRun() const { return simulationRun; }
    float speed() const { return settings.accessesPerSecond; }

    bool paused = false;
private:
    void performAccess()
    {
        const int address = pattern->nextAddress(memory);
        const CacheAccess cacheAccess = cache.access(address, tick++);
        const int registerSlot = registers.write(address, memory[address].value);

        metrics.totalAccesses += 1;
        const int accessCycles = cacheAccess.hit ? settings.hitCycles : settings.missCycles;
        if (cacheAccess.hit)
        {
            metrics.cacheHits += 1;
            metrics.estimatedCycles += accessCycles;
        }
        else
        {
            metrics.cacheMisses += 1;
            metrics.estimatedCycles += accessCycles;
        }
        consecutiveRegisterLoads += 1;

        accessHistory.push(AccessHistoryEntry{
            cacheAccess.hit,
            cacheAccess.evictedLineStart.has_value(),
            address,
            accessCycles
        });
        performanceHistory.push(PerformanceSample{
            metrics.hitRate(),
            metrics.averageCycles()
        });
        challenges.update(metrics.totalAccesses,
                          metrics.cacheMisses,
                          metrics.hitRate(),
                          metrics.averageCycles(),
                          consecutiveRegisterLoads);

        lastEvent = AccessEvent{
            address,
            cacheAccess.lineStart,
            cacheAccess.slotIndex,
            registerSlot,
            accessCycles,
            cacheAccess.hit,
            cacheAccess.evictedLineStart,
            0.0f
        };

        TraceEvent traceEvent{};
        traceEvent.tick = tick;
        traceEvent.type = cacheAccess.hit
            ? TraceEventType::CacheHit
            : TraceEventType::CacheMiss;
        traceEvent.address = address;
        traceEvent.lineStart = cacheAccess.lineStart;
        traceEvent.lineEnd = cacheAccess.lineStart + settings.cacheLineSize - 1;
        traceEvent.cacheLineIndex = cacheAccess.slotIndex;
        traceEvent.evictedLineStart = cacheAccess.evictedLineStart;
        traceEvent.simulatedCycles = accessCycles;

        metricsCollector.recordEvent(traceEvent);
        simulationRun.traceEvents.push_back(traceEvent);
        simulationRun.metrics = metricsCollector.getSnapshot();
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
    ExperimentSettings settings;
    AccessHistory accessHistory;
    PerformanceHistory performanceHistory;
    ChallengeSystem challenges;
    std::unique_ptr<AccessPattern> pattern;
    PatternKind patternKind = PatternKind::Sequential;
    std::optional<AccessEvent> lastEvent;
    float stepAccumulator = 0.0f;
    int consecutiveRegisterLoads = 0;
    int tick = 0;

    MetricsCollector metricsCollector;
    SimulationRun simulationRun;
};
}

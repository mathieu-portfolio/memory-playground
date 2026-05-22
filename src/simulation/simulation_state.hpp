#pragma once

#include "simulation/access_pattern.hpp"
#include "simulation/benchmark.hpp"
#include "simulation/cache.hpp"
#include "simulation/challenge.hpp"
#include "simulation/experiment_settings.hpp"
#include "simulation/history.hpp"
#include "simulation/instrumentation.hpp"
#include "simulation/metrics.hpp"
#include "simulation/register_file.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <optional>
#include <fstream>
#include <random>
#include <string>
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
        metricsCollector.reset();
        traceEvents.clear();
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

    void runBenchmarks()
    {
        BenchmarkRunner runner;
        latestBenchmarkReport = runner.runAll(defaultScenarioDefinitions());
        latestBenchmarkCsv = benchmarkReportToCsv(latestBenchmarkReport);
        hasBenchmarkReport = true;
    }

    bool exportLatestBenchmarkCsv(const std::string& path = "memory_playground_benchmarks.csv") const
    {
        if (!hasBenchmarkReport)
        {
            return false;
        }

        std::ofstream file(path);
        if (!file)
        {
            return false;
        }
        file << latestBenchmarkCsv;
        return true;
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
    const std::vector<TraceEvent>& getTraceEvents() const { return traceEvents; }
    MetricsSnapshot getMetricsSnapshot() const { return metricsCollector.snapshot(); }
    bool hasBenchmarks() const { return hasBenchmarkReport; }
    const BenchmarkReport& getBenchmarkReport() const { return latestBenchmarkReport; }
    const std::string& getBenchmarkCsv() const { return latestBenchmarkCsv; }
    float speed() const { return settings.accessesPerSecond; }

    bool paused = false;
private:
    void performAccess()
    {
        const int eventTick = tick++;
        const int address = pattern->nextAddress(memory);
        const CacheAccess cacheAccess = cache.access(address, eventTick);
        const int registerSlot = registers.write(address, memory[address].value);
        const int accessCycles = cacheAccess.hit ? settings.hitCycles : settings.missCycles;
        const int lineEnd = cacheAccess.lineStart + settings.cacheLineSize - 1;

        appendTrace(TraceEvent{eventTick,
                               TraceEventType::Read,
                               address,
                               cacheAccess.lineStart,
                               lineEnd,
                               cacheAccess.slotIndex,
                               registerSlot,
                               std::nullopt,
                               0,
                               "live_access"});
        appendTrace(TraceEvent{eventTick,
                               cacheAccess.hit ? TraceEventType::CacheHit : TraceEventType::CacheMiss,
                               address,
                               cacheAccess.lineStart,
                               lineEnd,
                               cacheAccess.slotIndex,
                               registerSlot,
                               std::nullopt,
                               accessCycles,
                               cacheAccess.hit ? "hit" : "miss"});
        if (cacheAccess.evictedLineStart.has_value())
        {
            appendTrace(TraceEvent{eventTick,
                                   TraceEventType::Eviction,
                                   address,
                                   cacheAccess.lineStart,
                                   lineEnd,
                                   cacheAccess.slotIndex,
                                   registerSlot,
                                   cacheAccess.evictedLineStart,
                                   0,
                                   "fifo_eviction"});
        }
        appendTrace(TraceEvent{eventTick,
                               TraceEventType::RegisterUpdate,
                               address,
                               cacheAccess.lineStart,
                               lineEnd,
                               cacheAccess.slotIndex,
                               registerSlot,
                               std::nullopt,
                               0,
                               "register_write"});

        metrics.totalAccesses += 1;
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
    }

    void appendTrace(const TraceEvent& event)
    {
        traceEvents.push_back(event);
        metricsCollector.recordEvent(event);
        constexpr std::size_t maxLiveTraceEvents = 2048;
        if (traceEvents.size() > maxLiveTraceEvents)
        {
            traceEvents.erase(traceEvents.begin(), traceEvents.begin() + static_cast<std::ptrdiff_t>(traceEvents.size() - maxLiveTraceEvents));
        }
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
    MetricsCollector metricsCollector;
    std::vector<TraceEvent> traceEvents;
    BenchmarkReport latestBenchmarkReport;
    std::string latestBenchmarkCsv;
    bool hasBenchmarkReport = false;
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
};
}

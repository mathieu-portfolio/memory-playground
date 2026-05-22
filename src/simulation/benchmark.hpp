#pragma once

#include "simulation/cache.hpp"
#include "simulation/constants.hpp"
#include "simulation/memory.hpp"
#include "simulation/metrics.hpp"
#include "simulation/scenario_definition.hpp"
#include "simulation/simulation_run.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace memory_playground
{
struct BenchmarkResult
{
    std::string scenarioName;
    std::string description;
    MetricsSnapshot metrics{};
    double durationMilliseconds = 0.0;
};

struct BenchmarkReport
{
    std::vector<BenchmarkResult> results;
};

class ScenarioAddressGenerator
{
public:
    explicit ScenarioAddressGenerator(const ScenarioDefinition& scenario)
        : definition(scenario), randomEngine(scenario.seed)
    {
        const int start = definition.clampedAddressStart();
        const int range = definition.clampedAddressRange();
        randomDistribution = std::uniform_int_distribution<int>(start, start + range - 1);
    }

    int next()
    {
        const int start = definition.clampedAddressStart();
        const int range = definition.clampedAddressRange();
        switch (definition.pattern)
        {
        case ScenarioPatternType::Sequential:
            return wrap(start + cursor++, start, range);
        case ScenarioPatternType::Random:
            return randomDistribution(randomEngine);
        case ScenarioPatternType::Stride:
        {
            const int step = std::max(1, definition.stride);
            const int address = wrap(start + cursor, start, range);
            cursor += step;
            return address;
        }
        case ScenarioPatternType::CacheThrash:
        {
            const int lineSize = std::max(1, definition.settings.cacheLineSize);
            const int lineCount = std::max(1, definition.settings.cacheLineCount + 1);
            const int line = cursor++ % lineCount;
            return wrap(start + line * lineSize, start, range);
        }
        case ScenarioPatternType::RepeatedHotSet:
        {
            const int hotSet = std::clamp(definition.hotSetSize, 1, range);
            const int address = start + (cursor++ % hotSet);
            return wrap(address, start, range);
        }
        case ScenarioPatternType::StructOfArrays:
            return wrap(start + cursor++, start, range);
        case ScenarioPatternType::ArrayOfStructs:
        {
            const int step = std::max(2, definition.stride);
            const int address = wrap(start + cursor, start, range);
            cursor += step;
            return address;
        }
        }
        return start;
    }

private:
    static int wrap(int address, int start, int range)
    {
        const int offset = (address - start) % range;
        return start + (offset < 0 ? offset + range : offset);
    }

    ScenarioDefinition definition;
    int cursor = 0;
    std::mt19937 randomEngine;
    std::uniform_int_distribution<int> randomDistribution{0, kRamCellCount - 1};
};

class BenchmarkRunner
{
public:
    SimulationRun runScenario(const ScenarioDefinition& scenario) const
    {
        std::vector<MemoryCell> memory;
        memory.reserve(kRamCellCount);
        for (int i = 0; i < kRamCellCount; ++i)
        {
            memory.push_back(MemoryCell{i, 1000 + i, 0});
        }

        SimpleCache cache;
        ExperimentSettings settings = scenario.settings;
        clampSettings(settings);
        cache.configure(settings.cacheLineSize, settings.cacheLineCount);
        cache.reset();

        MetricsCollector collector;
        ScenarioAddressGenerator generator(scenario);
        SimulationRun run;
        run.scenarioName = scenario.name;
        run.seed = scenario.seed;
        run.settings = settings;
        run.trace.reserve(static_cast<std::size_t>(std::max(0, scenario.steps)) * 3U);

        const auto started = std::chrono::steady_clock::now();
        for (int tick = 0; tick < scenario.steps; ++tick)
        {
            const int address = generator.next();
            const CacheAccess access = cache.access(address, tick);
            const int cycles = access.hit ? settings.hitCycles : settings.missCycles;
            const int lineEnd = access.lineStart + settings.cacheLineSize - 1;

            appendAndRecord(run, collector, TraceEvent{tick,
                                                       TraceEventType::Read,
                                                       address,
                                                       access.lineStart,
                                                       lineEnd,
                                                       access.slotIndex,
                                                       -1,
                                                       std::nullopt,
                                                       0,
                                                       "scenario_access"});

            appendAndRecord(run, collector, TraceEvent{tick,
                                                       access.hit ? TraceEventType::CacheHit : TraceEventType::CacheMiss,
                                                       address,
                                                       access.lineStart,
                                                       lineEnd,
                                                       access.slotIndex,
                                                       -1,
                                                       std::nullopt,
                                                       cycles,
                                                       access.hit ? "hit" : "miss"});

            if (access.evictedLineStart.has_value())
            {
                appendAndRecord(run, collector, TraceEvent{tick,
                                                           TraceEventType::Eviction,
                                                           address,
                                                           access.lineStart,
                                                           lineEnd,
                                                           access.slotIndex,
                                                           -1,
                                                           access.evictedLineStart,
                                                           0,
                                                           "fifo_eviction"});
            }
        }

        const auto finished = std::chrono::steady_clock::now();
        run.durationMilliseconds = std::chrono::duration<double, std::milli>(finished - started).count();
        run.finalMetrics = collector.snapshot();
        return run;
    }

    BenchmarkReport runAll(const std::vector<ScenarioDefinition>& scenarios) const
    {
        BenchmarkReport report;
        report.results.reserve(scenarios.size());
        for (const ScenarioDefinition& scenario : scenarios)
        {
            const SimulationRun run = runScenario(scenario);
            report.results.push_back(BenchmarkResult{scenario.name,
                                                     scenario.description,
                                                     run.finalMetrics,
                                                     run.durationMilliseconds});
        }
        return report;
    }

private:
    static void appendAndRecord(SimulationRun& run, MetricsCollector& collector, const TraceEvent& event)
    {
        run.trace.push_back(event);
        collector.recordEvent(event);
    }
};

inline std::string benchmarkReportToCsv(const BenchmarkReport& report)
{
    std::ostringstream out;
    out << "scenario,total_accesses,hits,misses,hit_rate,miss_rate,evictions,estimated_cycles,average_cycles,duration_ms\n";
    for (const BenchmarkResult& result : report.results)
    {
        out << result.scenarioName << ','
            << result.metrics.totalAccesses << ','
            << result.metrics.cacheHits << ','
            << result.metrics.cacheMisses << ','
            << result.metrics.hitRate() << ','
            << result.metrics.missRate() << ','
            << result.metrics.evictions << ','
            << result.metrics.estimatedCycles << ','
            << result.metrics.averageCycles() << ','
            << result.durationMilliseconds << '\n';
    }
    return out.str();
}
}

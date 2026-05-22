#include "simulation.hpp"
#include "simulation/benchmark.hpp"

#include <gtest/gtest.h>

#include <set>
#include <string>

using namespace memory_playground;

TEST(SimulationState, SequentialTraversalShowsSpatialLocality)
{
    SimulationState simulation;
    simulation.paused = true;

    for (int i = 0; i < kCacheLineSize; ++i)
    {
        simulation.stepOnce();
    }

    const Metrics& metrics = simulation.getMetrics();
    EXPECT_EQ(metrics.totalAccesses, 8);
    EXPECT_EQ(metrics.cacheMisses, 1);
    EXPECT_EQ(metrics.cacheHits, 7);
    EXPECT_EQ(metrics.estimatedCycles, kMissCycles + 7 * kHitCycles);
    EXPECT_NEAR(metrics.hitRate(), 87.5f, 0.001f);
}

TEST(SimpleCache, UsesFifoEviction)
{
    SimpleCache cache;

    for (int line = 0; line < kCacheLineCount; ++line)
    {
        const CacheAccess access = cache.access(line * kCacheLineSize, line);
        EXPECT_FALSE(access.hit);
        EXPECT_FALSE(access.evictedLineStart.has_value());
        EXPECT_EQ(access.slotIndex, line);
    }

    const CacheAccess eviction = cache.access(kCacheLineCount * kCacheLineSize, kCacheLineCount);
    EXPECT_FALSE(eviction.hit);
    ASSERT_TRUE(eviction.evictedLineStart.has_value());
    EXPECT_EQ(*eviction.evictedLineStart, 0);
    EXPECT_EQ(eviction.slotIndex, 0);

    const CacheAccess reaccessFirstLine = cache.access(0, kCacheLineCount + 1);
    EXPECT_FALSE(reaccessFirstLine.hit);
    ASSERT_TRUE(reaccessFirstLine.evictedLineStart.has_value());
    EXPECT_EQ(*reaccessFirstLine.evictedLineStart, kCacheLineSize);
}

TEST(RegisterFile, RotatesThroughSlots)
{
    RegisterFile registers;

    for (int i = 0; i < kRegisterCount + 1; ++i)
    {
        const int slot = registers.write(i, 1000 + i);
        EXPECT_EQ(slot, i % kRegisterCount);
    }

    const auto& slots = registers.getSlots();
    ASSERT_TRUE(slots[0].valid);
    EXPECT_EQ(slots[0].address, kRegisterCount);
    EXPECT_EQ(slots[0].value, 1000 + kRegisterCount);
}

TEST(SimulationState, LinkedListVisitsEveryCell)
{
    SimulationState simulation;
    simulation.setPattern(PatternKind::LinkedList);
    simulation.paused = true;

    std::set<int> visited;
    for (int i = 0; i < kRamCellCount; ++i)
    {
        simulation.stepOnce();
        const auto& event = simulation.getLastEvent();
        ASSERT_TRUE(event.has_value());
        visited.insert(event->address);
    }

    EXPECT_EQ(static_cast<int>(visited.size()), kRamCellCount);
}

TEST(SimulationState, SettingsResetSimulation)
{
    SimulationState simulation;
    simulation.paused = true;
    simulation.stepOnce();
    EXPECT_EQ(simulation.getMetrics().totalAccesses, 1);

    simulation.adjustCacheLineSize(1);
    EXPECT_EQ(simulation.getSettings().cacheLineSize, kDefaultCacheLineSize + 1);
    EXPECT_EQ(simulation.getMetrics().totalAccesses, 0);
    EXPECT_TRUE(simulation.getAccessHistory().getEntries().empty());
}

TEST(SimulationState, RecordsHistoryAndPerformanceSamples)
{
    SimulationState simulation;
    simulation.paused = true;
    simulation.stepOnce();
    simulation.stepOnce();

    EXPECT_EQ(simulation.getAccessHistory().getEntries().size(), 2U);
    EXPECT_EQ(simulation.getPerformanceHistory().getSamples().size(), 2U);
    EXPECT_FALSE(simulation.getChallenges().getChallenges().empty());
}

TEST(BenchmarkRunner, SequentialScenarioIsDeterministic)
{
    ScenarioDefinition scenario;
    scenario.name = "deterministic_sequential";
    scenario.pattern = ScenarioPatternType::Sequential;
    scenario.steps = kCacheLineSize;

    BenchmarkRunner runner;
    const SimulationRun first = runner.runScenario(scenario);
    const SimulationRun second = runner.runScenario(scenario);

    EXPECT_EQ(first.finalMetrics.totalAccesses, kCacheLineSize);
    EXPECT_EQ(first.finalMetrics.cacheMisses, 1);
    EXPECT_EQ(first.finalMetrics.cacheHits, kCacheLineSize - 1);
    ASSERT_EQ(first.trace.size(), second.trace.size());
    for (std::size_t i = 0; i < first.trace.size(); ++i)
    {
        EXPECT_EQ(first.trace[i].type, second.trace[i].type);
        EXPECT_EQ(first.trace[i].address, second.trace[i].address);
        EXPECT_EQ(first.trace[i].cacheLineStart, second.trace[i].cacheLineStart);
    }
}

TEST(BenchmarkRunner, CacheThrashProducesEvictions)
{
    ScenarioDefinition scenario;
    scenario.name = "thrash_test";
    scenario.pattern = ScenarioPatternType::CacheThrash;
    scenario.steps = (kDefaultCacheLineCount + 1) * 2;

    BenchmarkRunner runner;
    const SimulationRun run = runner.runScenario(scenario);

    EXPECT_EQ(run.finalMetrics.totalAccesses, scenario.steps);
    EXPECT_GT(run.finalMetrics.evictions, 0);
    EXPECT_EQ(run.finalMetrics.cacheHits, 0);
}

TEST(BenchmarkRunner, DefaultReportComparesScenarios)
{
    BenchmarkRunner runner;
    const BenchmarkReport report = runner.runAll(defaultScenarioDefinitions());

    ASSERT_GE(report.results.size(), 5U);

    const auto findResult = [&](const std::string& name) -> const BenchmarkResult* {
        for (const BenchmarkResult& result : report.results)
        {
            if (result.scenarioName == name)
            {
                return &result;
            }
        }
        return nullptr;
    };

    const BenchmarkResult* sequential = findResult("sequential_access");
    const BenchmarkResult* random = findResult("random_access");
    ASSERT_NE(sequential, nullptr);
    ASSERT_NE(random, nullptr);
    EXPECT_GT(sequential->metrics.hitRate(), random->metrics.hitRate());

    const std::string csv = benchmarkReportToCsv(report);
    EXPECT_NE(csv.find("scenario,total_accesses"), std::string::npos);
    EXPECT_NE(csv.find("sequential_access"), std::string::npos);
}

#include "simulation/trace_analysis.hpp"

TEST(SimulationState, RecordsLiveTraceForAdvancedVisualization)
{
    SimulationState simulation;
    simulation.paused = true;
    simulation.stepOnce();

    const auto& trace = simulation.getTraceEvents();
    ASSERT_GE(trace.size(), 3U);
    EXPECT_EQ(trace[0].type, TraceEventType::Read);
    EXPECT_EQ(trace[1].type, TraceEventType::CacheMiss);
    EXPECT_EQ(simulation.getMetricsSnapshot().totalAccesses, 1);
    EXPECT_EQ(simulation.getMetricsSnapshot().cacheMisses, 1);
}

TEST(TraceAnalysis, BucketsHitsMissesAndPressure)
{
    std::vector<TraceEvent> trace;
    trace.push_back(TraceEvent{0, TraceEventType::Read, 0, 0, 7, 0, -1, std::nullopt, 0, "read"});
    trace.push_back(TraceEvent{0, TraceEventType::CacheMiss, 0, 0, 7, 0, -1, std::nullopt, 10, "miss"});
    trace.push_back(TraceEvent{1, TraceEventType::Read, 1, 0, 7, 0, -1, std::nullopt, 0, "read"});
    trace.push_back(TraceEvent{1, TraceEventType::CacheHit, 1, 0, 7, 0, -1, std::nullopt, 1, "hit"});
    trace.push_back(TraceEvent{2, TraceEventType::Eviction, 16, 16, 23, 1, -1, 0, 0, "evict"});

    const TraceAnalysis analysis = analyzeTrace(trace, 2);
    ASSERT_EQ(analysis.buckets.size(), 2U);
    EXPECT_EQ(analysis.buckets[0].misses, 1);
    EXPECT_EQ(analysis.buckets[0].hits, 1);
    EXPECT_GT(analysis.buckets[1].evictions, 0);
    EXPECT_TRUE(analysis.latest.valid);
    EXPECT_TRUE(analysis.latest.eviction);
}

TEST(SimulationState, RunsAndExportsBenchmarkSuite)
{
    SimulationState simulation;

    EXPECT_FALSE(simulation.hasBenchmarks());
    simulation.runBenchmarks();

    ASSERT_TRUE(simulation.hasBenchmarks());
    EXPECT_GE(simulation.getBenchmarkReport().results.size(), 5U);
    EXPECT_NE(simulation.getBenchmarkCsv().find("sequential_access"), std::string::npos);
}

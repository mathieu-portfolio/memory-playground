#pragma once

#include "simulation/constants.hpp"
#include "simulation/instrumentation.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace memory_playground
{
struct TimelineBucket
{
    int firstTick = 0;
    int lastTick = 0;
    int reads = 0;
    int hits = 0;
    int misses = 0;
    int evictions = 0;
    int cycles = 0;

    int accesses() const { return hits + misses; }

    float hitRate() const
    {
        const int total = accesses();
        if (total == 0)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<float>(hits) / static_cast<float>(total);
    }

    float missRate() const
    {
        const int total = accesses();
        if (total == 0)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<float>(misses) / static_cast<float>(total);
    }

    float cachePressure() const
    {
        const int total = std::max(1, accesses());
        const float missPressure = static_cast<float>(misses) / static_cast<float>(total);
        const float evictionPressure = static_cast<float>(evictions) / static_cast<float>(total);
        return std::clamp(missPressure * 0.7f + evictionPressure * 0.3f, 0.0f, 1.0f);
    }
};

struct TraceInspection
{
    bool valid = false;
    int tick = 0;
    int address = -1;
    int cacheLineStart = -1;
    int cacheLineEnd = -1;
    int cacheSlot = -1;
    int cycles = 0;
    bool hit = false;
    bool eviction = false;
};

struct TraceAnalysis
{
    std::vector<TimelineBucket> buckets;
    TraceInspection latest;
    int maxBucketCycles = 1;
    int maxBucketEvents = 1;
};

inline TraceAnalysis analyzeTrace(const std::vector<TraceEvent>& trace, int bucketCount)
{
    TraceAnalysis analysis;
    if (trace.empty() || bucketCount <= 0)
    {
        return analysis;
    }

    const int firstTick = trace.front().tick;
    const int lastTick = trace.back().tick;
    const int tickSpan = std::max(1, lastTick - firstTick + 1);
    const int actualBucketCount = std::min(bucketCount, tickSpan);
    const int ticksPerBucket = std::max(1, static_cast<int>(std::ceil(static_cast<float>(tickSpan) / static_cast<float>(actualBucketCount))));

    analysis.buckets.resize(static_cast<std::size_t>(actualBucketCount));
    for (int i = 0; i < actualBucketCount; ++i)
    {
        analysis.buckets[static_cast<std::size_t>(i)].firstTick = firstTick + i * ticksPerBucket;
        analysis.buckets[static_cast<std::size_t>(i)].lastTick = firstTick + (i + 1) * ticksPerBucket - 1;
    }

    for (const TraceEvent& event : trace)
    {
        const int bucketIndex = std::clamp((event.tick - firstTick) / ticksPerBucket, 0, actualBucketCount - 1);
        TimelineBucket& bucket = analysis.buckets[static_cast<std::size_t>(bucketIndex)];

        switch (event.type)
        {
        case TraceEventType::Read:
            bucket.reads += 1;
            analysis.latest.valid = true;
            analysis.latest.tick = event.tick;
            analysis.latest.address = event.address;
            analysis.latest.cacheLineStart = event.cacheLineStart;
            analysis.latest.cacheLineEnd = event.cacheLineEnd;
            analysis.latest.cacheSlot = event.cacheSlot;
            break;
        case TraceEventType::CacheHit:
            bucket.hits += 1;
            bucket.cycles += event.simulatedCycles;
            analysis.latest.valid = true;
            analysis.latest.hit = true;
            analysis.latest.cycles = event.simulatedCycles;
            break;
        case TraceEventType::CacheMiss:
            bucket.misses += 1;
            bucket.cycles += event.simulatedCycles;
            analysis.latest.valid = true;
            analysis.latest.hit = false;
            analysis.latest.cycles = event.simulatedCycles;
            break;
        case TraceEventType::Eviction:
            bucket.evictions += 1;
            analysis.latest.valid = true;
            analysis.latest.eviction = true;
            break;
        case TraceEventType::Write:
        case TraceEventType::LoadLine:
        case TraceEventType::RegisterUpdate:
            break;
        }

        analysis.maxBucketCycles = std::max(analysis.maxBucketCycles, bucket.cycles);
        analysis.maxBucketEvents = std::max(analysis.maxBucketEvents, bucket.reads + bucket.hits + bucket.misses + bucket.evictions);
    }

    return analysis;
}

inline float normalizedHeat(int value, int maxValue)
{
    if (maxValue <= 0)
    {
        return 0.0f;
    }
    return std::clamp(static_cast<float>(value) / static_cast<float>(maxValue), 0.0f, 1.0f);
}
}

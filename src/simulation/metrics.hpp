#pragma once

#include "simulation/instrumentation.hpp"

namespace memory_playground
{
struct Metrics
{
    int totalAccesses = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
    int estimatedCycles = 0;
    int reads = 0;
    int writes = 0;
    int evictions = 0;

    float hitRate() const
    {
        if (totalAccesses == 0)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<float>(cacheHits) / static_cast<float>(totalAccesses);
    }

    float missRate() const
    {
        if (totalAccesses == 0)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<float>(cacheMisses) / static_cast<float>(totalAccesses);
    }

    float averageCycles() const
    {
        if (totalAccesses == 0)
        {
            return 0.0f;
        }
        return static_cast<float>(estimatedCycles) / static_cast<float>(totalAccesses);
    }

    static Metrics fromSnapshot(const MetricsSnapshot& snapshot)
    {
        Metrics metrics;
        metrics.totalAccesses = snapshot.totalAccesses;
        metrics.cacheHits = snapshot.cacheHits;
        metrics.cacheMisses = snapshot.cacheMisses;
        metrics.estimatedCycles = snapshot.estimatedCycles;
        metrics.reads = snapshot.reads;
        metrics.writes = snapshot.writes;
        metrics.evictions = snapshot.evictions;
        return metrics;
    }
};
}

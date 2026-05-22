
#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace memory_playground
{
enum class TraceEventType
{
    Read,
    Write,
    CacheHit,
    CacheMiss,
    Eviction,
    LoadLine,
    RegisterUpdate
};

struct TraceEvent
{
    int tick = 0;
    TraceEventType type = TraceEventType::Read;
    int address = 0;
    int lineStart = 0;
    int lineEnd = 0;
    int cacheLineIndex = -1;
    std::optional<int> evictedLineStart;
    int simulatedCycles = 0;
    std::string label;
};

struct MetricsSnapshot
{
    int totalAccesses = 0;
    int reads = 0;
    int writes = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
    int evictions = 0;
    int simulatedCycles = 0;

    float hitRate = 0.0f;
    float missRate = 0.0f;
};

class MetricsCollector
{
public:
    void reset()
    {
        snapshot = MetricsSnapshot{};
        addressAccessCounts.clear();
        cacheLineAccessCounts.clear();
    }

    void recordEvent(const TraceEvent& event)
    {
        snapshot.totalAccesses++;

        if (event.type == TraceEventType::Read)
        {
            snapshot.reads++;
        }

        if (event.type == TraceEventType::CacheHit)
        {
            snapshot.cacheHits++;
        }

        if (event.type == TraceEventType::CacheMiss)
        {
            snapshot.cacheMisses++;
        }

        if (event.evictedLineStart.has_value())
        {
            snapshot.evictions++;
        }

        snapshot.simulatedCycles += event.simulatedCycles;

        addressAccessCounts[event.address]++;
        cacheLineAccessCounts[event.lineStart]++;

        if (snapshot.totalAccesses > 0)
        {
            snapshot.hitRate =
                100.0f * static_cast<float>(snapshot.cacheHits) /
                static_cast<float>(snapshot.totalAccesses);

            snapshot.missRate =
                100.0f * static_cast<float>(snapshot.cacheMisses) /
                static_cast<float>(snapshot.totalAccesses);
        }
    }

    const MetricsSnapshot& getSnapshot() const
    {
        return snapshot;
    }

    const std::unordered_map<int, int>& getAddressAccessCounts() const
    {
        return addressAccessCounts;
    }

    const std::unordered_map<int, int>& getCacheLineAccessCounts() const
    {
        return cacheLineAccessCounts;
    }

private:
    MetricsSnapshot snapshot;
    std::unordered_map<int, int> addressAccessCounts;
    std::unordered_map<int, int> cacheLineAccessCounts;
};
}

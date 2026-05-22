#pragma once

#include <map>
#include <optional>
#include <string>

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

inline const char* traceEventTypeName(TraceEventType type)
{
    switch (type)
    {
    case TraceEventType::Read: return "read";
    case TraceEventType::Write: return "write";
    case TraceEventType::CacheHit: return "cache_hit";
    case TraceEventType::CacheMiss: return "cache_miss";
    case TraceEventType::Eviction: return "eviction";
    case TraceEventType::LoadLine: return "load_line";
    case TraceEventType::RegisterUpdate: return "register_update";
    }
    return "unknown";
}

struct TraceEvent
{
    int tick = 0;
    TraceEventType type = TraceEventType::Read;
    int address = -1;
    int cacheLineStart = -1;
    int cacheLineEnd = -1;
    int cacheSlot = -1;
    int registerSlot = -1;
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
    int estimatedCycles = 0;
    std::map<int, int> perAddressAccessCounts;
    std::map<int, int> perCacheLineAccessCounts;

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
};

class MetricsCollector
{
public:
    void reset()
    {
        current = MetricsSnapshot{};
    }

    void recordEvent(const TraceEvent& event)
    {
        switch (event.type)
        {
        case TraceEventType::Read:
            current.totalAccesses += 1;
            current.reads += 1;
            recordAccessLocation(event);
            break;
        case TraceEventType::Write:
            current.totalAccesses += 1;
            current.writes += 1;
            recordAccessLocation(event);
            break;
        case TraceEventType::CacheHit:
            current.cacheHits += 1;
            current.estimatedCycles += event.simulatedCycles;
            break;
        case TraceEventType::CacheMiss:
            current.cacheMisses += 1;
            current.estimatedCycles += event.simulatedCycles;
            break;
        case TraceEventType::Eviction:
            current.evictions += 1;
            break;
        case TraceEventType::LoadLine:
        case TraceEventType::RegisterUpdate:
            break;
        }
    }

    MetricsSnapshot snapshot() const
    {
        return current;
    }

private:
    void recordAccessLocation(const TraceEvent& event)
    {
        if (event.address >= 0)
        {
            current.perAddressAccessCounts[event.address] += 1;
        }
        if (event.cacheLineStart >= 0)
        {
            current.perCacheLineAccessCounts[event.cacheLineStart] += 1;
        }
    }

    MetricsSnapshot current;
};
}

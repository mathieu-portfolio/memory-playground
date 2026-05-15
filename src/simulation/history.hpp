#pragma once

#include "simulation/constants.hpp"

#include <vector>

namespace memory_playground
{
struct AccessHistoryEntry
{
    bool hit = false;
    bool evicted = false;
    int address = 0;
    int cycles = 0;
};

class AccessHistory
{
public:
    void clear()
    {
        entries.clear();
    }

    void push(AccessHistoryEntry entry)
    {
        if (entries.size() >= kAccessHistorySize)
        {
            entries.erase(entries.begin());
        }
        entries.push_back(entry);
    }

    const std::vector<AccessHistoryEntry>& getEntries() const
    {
        return entries;
    }

private:
    std::vector<AccessHistoryEntry> entries;
};

struct PerformanceSample
{
    float hitRate = 0.0f;
    float averageCycles = 0.0f;
};

class PerformanceHistory
{
public:
    void clear()
    {
        samples.clear();
    }

    void push(PerformanceSample sample)
    {
        if (samples.size() >= kPerformanceHistorySize)
        {
            samples.erase(samples.begin());
        }
        samples.push_back(sample);
    }

    const std::vector<PerformanceSample>& getSamples() const
    {
        return samples;
    }

private:
    std::vector<PerformanceSample> samples;
};
}

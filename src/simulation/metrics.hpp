#pragma once

namespace memory_playground
{
struct Metrics
{
    int totalAccesses = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
    int estimatedCycles = 0;

    float hitRate() const
    {
        if (totalAccesses == 0)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<float>(cacheHits) / static_cast<float>(totalAccesses);
    }
};
}

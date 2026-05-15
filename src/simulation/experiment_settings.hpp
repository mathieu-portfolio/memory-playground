#pragma once

#include "simulation/constants.hpp"

#include <algorithm>

namespace memory_playground
{
struct ExperimentSettings
{
    float accessesPerSecond = 2.0f;
    int cacheLineSize = kDefaultCacheLineSize;
    int cacheLineCount = kDefaultCacheLineCount;
    int hitCycles = kDefaultHitCycles;
    int missCycles = kDefaultMissCycles;
};

inline void clampSettings(ExperimentSettings& settings)
{
    settings.accessesPerSecond = std::clamp(settings.accessesPerSecond, 0.5f, 12.0f);
    settings.cacheLineSize = std::clamp(settings.cacheLineSize, kMinCacheLineSize, kMaxCacheLineSize);
    settings.cacheLineCount = std::clamp(settings.cacheLineCount, kMinCacheLineCount, kMaxCacheLineCount);
    settings.hitCycles = std::clamp(settings.hitCycles, kMinHitCycles, kMaxHitCycles);
    settings.missCycles = std::clamp(settings.missCycles, kMinMissCycles, kMaxMissCycles);
}
}

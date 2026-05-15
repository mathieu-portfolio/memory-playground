#pragma once

namespace memory_playground
{
constexpr int kRamCellCount = 128;
constexpr int kDefaultCacheLineSize = 8;
constexpr int kDefaultCacheLineCount = 8;
constexpr int kMinCacheLineSize = 2;
constexpr int kMaxCacheLineSize = 32;
constexpr int kMinCacheLineCount = 2;
constexpr int kMaxCacheLineCount = 16;
constexpr int kRegisterCount = 4;
constexpr int kDefaultHitCycles = 4;
constexpr int kDefaultMissCycles = 100;
constexpr int kMinHitCycles = 1;
constexpr int kMaxHitCycles = 20;
constexpr int kMinMissCycles = 20;
constexpr int kMaxMissCycles = 300;
constexpr int kAccessHistorySize = 48;
constexpr int kPerformanceHistorySize = 96;

// Backward-compatible aliases for tests and simple examples.
constexpr int kCacheLineSize = kDefaultCacheLineSize;
constexpr int kCacheLineCount = kDefaultCacheLineCount;
constexpr int kHitCycles = kDefaultHitCycles;
constexpr int kMissCycles = kDefaultMissCycles;
}

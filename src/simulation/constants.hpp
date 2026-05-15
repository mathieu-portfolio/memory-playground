#pragma once

namespace memory_playground
{
constexpr int kRamCellCount = 128;
constexpr int kCacheLineSize = 8;
constexpr int kCacheLineCount = 8;
constexpr int kRegisterCount = 4;
constexpr int kHitCycles = 4;
constexpr int kMissCycles = 100;
}

#pragma once

#include "simulation/constants.hpp"

#include <algorithm>
#include <array>
#include <optional>

namespace memory_playground
{
enum class CacheFlashKind
{
    Neutral,
    Hit,
    Miss
};

struct CacheSlot
{
    bool valid = false;
    int lineStart = -1;
    int loadedAt = 0;
    float flash = 0.0f;
    CacheFlashKind flashKind = CacheFlashKind::Neutral;
};

struct CacheAccess
{
    bool hit = false;
    int slotIndex = 0;
    int lineStart = 0;
    std::optional<int> evictedLineStart;
};

class SimpleCache
{
public:
    CacheAccess access(int address, int tick)
    {
        const int lineStart = (address / kCacheLineSize) * kCacheLineSize;

        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            if (slots[i].valid && slots[i].lineStart == lineStart)
            {
                slots[i].flash = 0.45f;
                slots[i].flashKind = CacheFlashKind::Hit;
                return CacheAccess{true, i, lineStart, std::nullopt};
            }
        }

        int targetSlot = -1;
        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            if (!slots[i].valid)
            {
                targetSlot = i;
                break;
            }
        }

        if (targetSlot == -1)
        {
            // FIFO is intentionally used instead of LRU because it is easy to see:
            // misses replace slots in a fixed rotating order.
            targetSlot = nextEvictionSlot;
            nextEvictionSlot = (nextEvictionSlot + 1) % static_cast<int>(slots.size());
        }

        std::optional<int> evicted;
        if (slots[targetSlot].valid)
        {
            evicted = slots[targetSlot].lineStart;
        }

        slots[targetSlot] = CacheSlot{true, lineStart, tick, 0.65f, CacheFlashKind::Miss};
        return CacheAccess{false, targetSlot, lineStart, evicted};
    }

    void reset()
    {
        for (auto& slot : slots)
        {
            slot = CacheSlot{};
        }
        nextEvictionSlot = 0;
    }

    void update(float dt)
    {
        for (auto& slot : slots)
        {
            slot.flash = std::max(0.0f, slot.flash - dt);
        }
    }

    const std::array<CacheSlot, kCacheLineCount>& getSlots() const
    {
        return slots;
    }

private:
    std::array<CacheSlot, kCacheLineCount> slots{};
    int nextEvictionSlot = 0;
};
}

#include "simulation.hpp"

#include <cassert>
#include <cmath>
#include <set>

using namespace memory_playground;

namespace
{
void testSequentialSpatialLocality()
{
    SimulationState simulation;
    simulation.paused = true;

    for (int i = 0; i < kCacheLineSize; ++i)
    {
        simulation.stepOnce();
    }

    const Metrics& metrics = simulation.getMetrics();
    assert(metrics.totalAccesses == 8);
    assert(metrics.cacheMisses == 1);
    assert(metrics.cacheHits == 7);
    assert(metrics.estimatedCycles == kMissCycles + 7 * kHitCycles);
    assert(std::fabs(metrics.hitRate() - 87.5f) < 0.001f);
}

void testFifoEviction()
{
    SimpleCache cache;

    for (int line = 0; line < kCacheLineCount; ++line)
    {
        const CacheAccess access = cache.access(line * kCacheLineSize, line);
        assert(!access.hit);
        assert(!access.evictedLineStart);
        assert(access.slotIndex == line);
    }

    const CacheAccess eviction = cache.access(kCacheLineCount * kCacheLineSize, kCacheLineCount);
    assert(!eviction.hit);
    assert(eviction.evictedLineStart);
    assert(*eviction.evictedLineStart == 0);
    assert(eviction.slotIndex == 0);

    const CacheAccess reaccessFirstLine = cache.access(0, kCacheLineCount + 1);
    assert(!reaccessFirstLine.hit);
    assert(reaccessFirstLine.evictedLineStart);
    assert(*reaccessFirstLine.evictedLineStart == kCacheLineSize);
}

void testRegisterRotation()
{
    RegisterFile registers;

    for (int i = 0; i < kRegisterCount + 1; ++i)
    {
        const int slot = registers.write(i, 1000 + i);
        assert(slot == i % kRegisterCount);
    }

    const auto& slots = registers.getSlots();
    assert(slots[0].valid);
    assert(slots[0].address == kRegisterCount);
    assert(slots[0].value == 1000 + kRegisterCount);
}

void testLinkedListVisitsEveryCell()
{
    SimulationState simulation;
    simulation.setPattern(PatternKind::LinkedList);
    simulation.paused = true;

    std::set<int> visited;
    for (int i = 0; i < kRamCellCount; ++i)
    {
        simulation.stepOnce();
        const auto& event = simulation.getLastEvent();
        assert(event);
        visited.insert(event->address);
    }

    assert(static_cast<int>(visited.size()) == kRamCellCount);
}
}

int main()
{
    testSequentialSpatialLocality();
    testFifoEviction();
    testRegisterRotation();
    testLinkedListVisitsEveryCell();
    return 0;
}

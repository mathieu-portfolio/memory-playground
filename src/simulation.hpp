#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <vector>

namespace memory_playground
{
constexpr int kRamCellCount = 128;
constexpr int kCacheLineSize = 8;
constexpr int kCacheLineCount = 8;
constexpr int kRegisterCount = 4;
constexpr int kHitCycles = 4;
constexpr int kMissCycles = 100;

struct MemoryCell
{
    int address = 0;
    int value = 0;
    int nextAddress = 0;
};

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

struct RegisterSlot
{
    bool valid = false;
    int address = -1;
    int value = 0;
    float flash = 0.0f;
};

class RegisterFile
{
public:
    int write(int address, int value)
    {
        const int slot = nextSlot;
        slots[slot] = RegisterSlot{true, address, value, 0.6f};
        nextSlot = (nextSlot + 1) % static_cast<int>(slots.size());
        return slot;
    }

    void reset()
    {
        for (auto& slot : slots)
        {
            slot = RegisterSlot{};
        }
        nextSlot = 0;
    }

    void update(float dt)
    {
        for (auto& slot : slots)
        {
            slot.flash = std::max(0.0f, slot.flash - dt);
        }
    }

    const std::array<RegisterSlot, kRegisterCount>& getSlots() const
    {
        return slots;
    }

private:
    std::array<RegisterSlot, kRegisterCount> slots{};
    int nextSlot = 0;
};

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

class AccessPattern
{
public:
    virtual ~AccessPattern() = default;
    virtual void reset(const std::vector<MemoryCell>& memory) = 0;
    virtual int nextAddress(const std::vector<MemoryCell>& memory) = 0;
    virtual const char* name() const = 0;
    virtual const char* description() const = 0;
};

class SequentialPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>&) override
    {
        cursor = 0;
    }

    int nextAddress(const std::vector<MemoryCell>& memory) override
    {
        const int address = cursor;
        cursor = (cursor + 1) % static_cast<int>(memory.size());
        return address;
    }

    const char* name() const override
    {
        return "Sequential Array Traversal";
    }

    const char* description() const override
    {
        return "Contiguous addresses reuse the cache line that was just loaded.";
    }

private:
    int cursor = 0;
};

class RandomPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>& memory) override
    {
        distribution = std::uniform_int_distribution<int>(0, static_cast<int>(memory.size()) - 1);
        generator.seed(7);
    }

    int nextAddress(const std::vector<MemoryCell>&) override
    {
        return distribution(generator);
    }

    const char* name() const override
    {
        return "Random Access Traversal";
    }

    const char* description() const override
    {
        return "Random jumps often land in cache lines that are not loaded.";
    }

private:
    std::mt19937 generator{7};
    std::uniform_int_distribution<int> distribution{0, kRamCellCount - 1};
};

class LinkedListPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>& memory) override
    {
        current = memory.empty() ? 0 : memory.front().address;
    }

    int nextAddress(const std::vector<MemoryCell>& memory) override
    {
        const int address = current;
        current = memory[address].nextAddress;
        return address;
    }

    const char* name() const override
    {
        return "Linked List Traversal";
    }

    const char* description() const override
    {
        return "Each node points to the next address, so traversal jumps around RAM.";
    }

private:
    int current = 0;
};

enum class PatternKind
{
    Sequential,
    Random,
    LinkedList
};

struct AccessEvent
{
    int address = 0;
    int lineStart = 0;
    int cacheSlot = 0;
    int registerSlot = 0;
    bool hit = false;
    std::optional<int> evictedLineStart;
    float age = 0.0f;
};

class SimulationState
{
public:
    SimulationState()
    {
        memory.reserve(kRamCellCount);
        for (int i = 0; i < kRamCellCount; ++i)
        {
            memory.push_back(MemoryCell{i, 1000 + i, 0});
        }
        buildLinkedList();
        setPattern(PatternKind::Sequential);
    }

    void setPattern(PatternKind kind)
    {
        patternKind = kind;
        switch (patternKind)
        {
        case PatternKind::Sequential:
            pattern = std::make_unique<SequentialPattern>();
            break;
        case PatternKind::Random:
            pattern = std::make_unique<RandomPattern>();
            break;
        case PatternKind::LinkedList:
            pattern = std::make_unique<LinkedListPattern>();
            break;
        }
        reset();
    }

    void reset()
    {
        cache.reset();
        registers.reset();
        metrics = Metrics{};
        tick = 0;
        stepAccumulator = 0.0f;
        lastEvent.reset();
        pattern->reset(memory);
    }

    void update(float dt)
    {
        cache.update(dt);
        registers.update(dt);
        if (lastEvent)
        {
            lastEvent->age += dt;
            if (lastEvent->age > 1.0f)
            {
                lastEvent.reset();
            }
        }

        if (paused)
        {
            return;
        }

        stepAccumulator += dt * accessesPerSecond;
        while (stepAccumulator >= 1.0f)
        {
            performAccess();
            stepAccumulator -= 1.0f;
        }
    }

    void stepOnce()
    {
        performAccess();
    }

    void faster()
    {
        accessesPerSecond = std::min(12.0f, accessesPerSecond + 0.5f);
    }

    void slower()
    {
        accessesPerSecond = std::max(0.5f, accessesPerSecond - 0.5f);
    }

    const std::vector<MemoryCell>& getMemory() const { return memory; }
    const SimpleCache& getCache() const { return cache; }
    const RegisterFile& getRegisters() const { return registers; }
    const Metrics& getMetrics() const { return metrics; }
    const AccessPattern& getPattern() const { return *pattern; }
    const std::optional<AccessEvent>& getLastEvent() const { return lastEvent; }
    float speed() const { return accessesPerSecond; }

    bool paused = false;
    bool showOverlay = true;

private:
    void performAccess()
    {
        const int address = pattern->nextAddress(memory);
        const CacheAccess cacheAccess = cache.access(address, tick++);
        const int registerSlot = registers.write(address, memory[address].value);

        metrics.totalAccesses += 1;
        if (cacheAccess.hit)
        {
            metrics.cacheHits += 1;
            metrics.estimatedCycles += kHitCycles;
        }
        else
        {
            metrics.cacheMisses += 1;
            metrics.estimatedCycles += kMissCycles;
        }

        lastEvent = AccessEvent{
            address,
            cacheAccess.lineStart,
            cacheAccess.slotIndex,
            registerSlot,
            cacheAccess.hit,
            cacheAccess.evictedLineStart,
            0.0f
        };
    }

    void buildLinkedList()
    {
        std::vector<int> addresses(kRamCellCount);
        std::iota(addresses.begin(), addresses.end(), 0);
        std::mt19937 generator{42};
        std::shuffle(addresses.begin(), addresses.end(), generator);

        for (int i = 0; i < kRamCellCount; ++i)
        {
            const int address = addresses[i];
            const int next = addresses[(i + 1) % kRamCellCount];
            memory[address].nextAddress = next;
        }
    }

    std::vector<MemoryCell> memory;
    SimpleCache cache;
    RegisterFile registers;
    Metrics metrics;
    std::unique_ptr<AccessPattern> pattern;
    PatternKind patternKind = PatternKind::Sequential;
    std::optional<AccessEvent> lastEvent;
    float accessesPerSecond = 2.0f;
    float stepAccumulator = 0.0f;
    int tick = 0;
};
}

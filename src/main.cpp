#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace
{
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 820;
constexpr int kRamCellCount = 64;
constexpr int kCacheLineSize = 8;
constexpr int kCacheLineCount = 8;
constexpr int kRegisterCount = 4;
constexpr int kHitCycles = 4;
constexpr int kMissCycles = 100;

constexpr Color kBackground{18, 22, 28, 255};
constexpr Color kPanel{31, 38, 48, 255};
constexpr Color kPanelBorder{67, 78, 92, 255};
constexpr Color kText{229, 236, 244, 255};
constexpr Color kMutedText{151, 163, 178, 255};
constexpr Color kRamColor{69, 99, 132, 255};
constexpr Color kCacheColor{53, 137, 108, 255};
constexpr Color kRegisterColor{144, 104, 190, 255};
constexpr Color kHitColor{74, 222, 128, 255};
constexpr Color kMissColor{248, 113, 113, 255};
constexpr Color kEvictColor{251, 191, 36, 255};
constexpr Color kLineColor{101, 116, 139, 255};

struct MemoryCell
{
    int address = 0;
    int value = 0;
    int nextAddress = 0;
};

struct CacheSlot
{
    bool valid = false;
    int lineStart = -1;
    int loadedAt = 0;
    float flash = 0.0f;
    Color flashColor = kCacheColor;
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
                slots[i].flashColor = kHitColor;
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

        slots[targetSlot] = CacheSlot{true, lineStart, tick, 0.65f, kMissColor};
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

class Renderer
{
public:
    void draw(const SimulationState& simulation)
    {
        BeginDrawing();
        ClearBackground(kBackground);

        drawTitle(simulation);
        drawRegisters(simulation);
        drawCache(simulation);
        drawRam(simulation);
        drawFlow(simulation);
        drawMetrics(simulation);

        if (simulation.showOverlay)
        {
            drawHelp(simulation);
        }

        EndDrawing();
    }

private:
    static Color fadeTo(Color base, Color highlight, float amount)
    {
        amount = std::clamp(amount, 0.0f, 1.0f);
        return Color{
            static_cast<unsigned char>(base.r + (highlight.r - base.r) * amount),
            static_cast<unsigned char>(base.g + (highlight.g - base.g) * amount),
            static_cast<unsigned char>(base.b + (highlight.b - base.b) * amount),
            255
        };
    }

    void drawPanel(Rectangle bounds, const char* title) const
    {
        DrawRectangleRounded(bounds, 0.06f, 8, kPanel);
        DrawRectangleRoundedLines(bounds, 0.06f, 8, kPanelBorder);
        DrawText(title, static_cast<int>(bounds.x) + 18, static_cast<int>(bounds.y) + 14, 22, kText);
    }

    void drawTitle(const SimulationState& simulation) const
    {
        DrawText("memory-playground", 30, 22, 28, kText);
        DrawText(simulation.getPattern().description(), 310, 30, 18, kMutedText);
    }

    void drawRegisters(const SimulationState& simulation) const
    {
        const Rectangle panel{30, 74, 760, 116};
        drawPanel(panel, "Registers");

        const auto& slots = simulation.getRegisters().getSlots();
        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            const float x = panel.x + 170 + i * 135.0f;
            const Rectangle slotRect{x, panel.y + 43, 104, 48};
            const Color fill = fadeTo(kRegisterColor, WHITE, slots[i].flash);
            DrawRectangleRounded(slotRect, 0.08f, 8, fill);
            DrawRectangleRoundedLines(slotRect, 0.08f, 8, kPanelBorder);

            char label[64];
            if (slots[i].valid)
            {
                std::snprintf(label, sizeof(label), "R%d  [%02d]", i, slots[i].address);
            }
            else
            {
                std::snprintf(label, sizeof(label), "R%d  --", i);
            }
            DrawText(label, static_cast<int>(x) + 13, static_cast<int>(slotRect.y) + 15, 18, kText);
        }
    }

    void drawCache(const SimulationState& simulation) const
    {
        const Rectangle panel{30, 214, 760, 210};
        drawPanel(panel, "L1 Cache (8 cache lines, FIFO eviction)");

        const auto& slots = simulation.getCache().getSlots();
        const auto& event = simulation.getLastEvent();
        for (int i = 0; i < static_cast<int>(slots.size()); ++i)
        {
            const float x = panel.x + 28 + (i % 4) * 180.0f;
            const float y = panel.y + 54 + (i / 4) * 70.0f;
            const Rectangle slotRect{x, y, 154, 50};

            Color fill = slots[i].valid ? kCacheColor : Color{45, 54, 65, 255};
            fill = fadeTo(fill, slots[i].flashColor, slots[i].flash);
            if (event && event->cacheSlot == i)
            {
                fill = fadeTo(fill, event->hit ? kHitColor : kMissColor, 0.25f);
            }

            DrawRectangleRounded(slotRect, 0.08f, 8, fill);
            DrawRectangleRoundedLines(slotRect, 0.08f, 8, kPanelBorder);

            char label[80];
            if (slots[i].valid)
            {
                std::snprintf(label, sizeof(label), "Slot %d: %02d-%02d", i, slots[i].lineStart, slots[i].lineStart + kCacheLineSize - 1);
            }
            else
            {
                std::snprintf(label, sizeof(label), "Slot %d: empty", i);
            }
            DrawText(label, static_cast<int>(x) + 12, static_cast<int>(y) + 16, 17, kText);
        }
    }

    void drawRam(const SimulationState& simulation) const
    {
        const Rectangle panel{30, 448, 1220, 298};
        drawPanel(panel, "RAM (64 cells grouped into 8-cell cache lines)");

        const auto& memory = simulation.getMemory();
        const auto& event = simulation.getLastEvent();
        const int rows = kRamCellCount / kCacheLineSize;
        const float cellW = 50.0f;
        const float cellH = 25.0f;
        const float startX = panel.x + 152.0f;
        const float startY = panel.y + 58.0f;

        for (int line = 0; line < rows; ++line)
        {
            const float y = startY + line * 28.0f;
            char lineLabel[32];
            std::snprintf(lineLabel, sizeof(lineLabel), "Line %d", line);
            DrawText(lineLabel, static_cast<int>(panel.x) + 38, static_cast<int>(y) + 5, 16, kMutedText);

            const Rectangle groupRect{startX - 6.0f, y - 5.0f, kCacheLineSize * cellW + 12.0f, cellH + 10.0f};
            const bool activeLine = event && event->lineStart == line * kCacheLineSize;
            DrawRectangleRoundedLines(groupRect, 0.05f, 6, activeLine ? kHitColor : kLineColor);

            for (int offset = 0; offset < kCacheLineSize; ++offset)
            {
                const int address = line * kCacheLineSize + offset;
                const float x = startX + offset * cellW;
                const Rectangle cellRect{x, y, cellW - 3.0f, cellH};

                Color fill = kRamColor;
                if (event && event->address == address)
                {
                    fill = event->hit ? kHitColor : kMissColor;
                }
                else if (event && event->lineStart == line * kCacheLineSize)
                {
                    fill = fadeTo(fill, event->hit ? kHitColor : kMissColor, 0.28f);
                }
                else if (event && event->evictedLineStart && *event->evictedLineStart == line * kCacheLineSize)
                {
                    fill = fadeTo(fill, kEvictColor, 0.35f);
                }

                DrawRectangleRounded(cellRect, 0.07f, 6, fill);
                char label[16];
                std::snprintf(label, sizeof(label), "%02d", memory[address].address);
                DrawText(label, static_cast<int>(x) + 13, static_cast<int>(y) + 5, 15, kText);
            }

            if (event && event->address >= 0 && event->address < static_cast<int>(memory.size()))
            {
                const int next = memory[event->address].nextAddress;
                if (line == event->address / kCacheLineSize)
                {
                    char nextLabel[64];
                    std::snprintf(nextLabel, sizeof(nextLabel), "next -> %02d", next);
                    DrawText(nextLabel, static_cast<int>(startX + kCacheLineSize * cellW + 28), static_cast<int>(y) + 5, 16, kMutedText);
                }
            }
        }

        DrawText("A miss loads the whole highlighted RAM cache line into L1, not just one cell.",
                 static_cast<int>(panel.x) + 38,
                 static_cast<int>(panel.y + panel.height - 38),
                 18,
                 kMutedText);
    }

    void drawFlow(const SimulationState& simulation) const
    {
        const auto& event = simulation.getLastEvent();
        if (!event)
        {
            return;
        }

        const float t = std::clamp(event->age / 0.7f, 0.0f, 1.0f);
        const Color color = event->hit ? kHitColor : kMissColor;
        const Vector2 from{965.0f, 590.0f};
        const Vector2 mid{860.0f, 320.0f};
        const Vector2 to{650.0f, 132.0f};

        Vector2 point = mid;
        if (t < 0.5f)
        {
            const float local = t / 0.5f;
            point = Vector2{from.x + (mid.x - from.x) * local, from.y + (mid.y - from.y) * local};
        }
        else
        {
            const float local = (t - 0.5f) / 0.5f;
            point = Vector2{mid.x + (to.x - mid.x) * local, mid.y + (to.y - mid.y) * local};
        }

        DrawLineEx(from, mid, 3.0f, Fade(color, 0.45f));
        DrawLineEx(mid, to, 3.0f, Fade(color, 0.45f));
        DrawCircleV(point, 10.0f, color);

        DrawText(event->hit ? "cache hit: L1 -> register" : "cache miss: RAM -> L1 -> register",
                 830,
                 262,
                 18,
                 color);
    }

    void drawMetrics(const SimulationState& simulation) const
    {
        const Rectangle panel{820, 74, 430, 350};
        drawPanel(panel, "Metrics");

        const Metrics& metrics = simulation.getMetrics();
        const auto& event = simulation.getLastEvent();

        char buffer[160];
        const int x = static_cast<int>(panel.x) + 24;
        int y = static_cast<int>(panel.y) + 58;

        DrawText(simulation.getPattern().name(), x, y, 20, kText);
        y += 38;

        std::snprintf(buffer, sizeof(buffer), "Accesses: %d", metrics.totalAccesses);
        DrawText(buffer, x, y, 18, kText);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Hits: %d", metrics.cacheHits);
        DrawText(buffer, x, y, 18, kHitColor);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Misses: %d", metrics.cacheMisses);
        DrawText(buffer, x, y, 18, kMissColor);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Hit rate: %.1f%%", metrics.hitRate());
        DrawText(buffer, x, y, 18, kText);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Estimated cycles: %d", metrics.estimatedCycles);
        DrawText(buffer, x, y, 18, kText);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Speed: %.1f accesses/sec", simulation.speed());
        DrawText(buffer, x, y, 18, kText);
        y += 36;

        if (event)
        {
            std::snprintf(buffer, sizeof(buffer), "Current address: %02d", event->address);
            DrawText(buffer, x, y, 18, kText);
            y += 28;
            std::snprintf(buffer, sizeof(buffer), "Cache line: %02d-%02d", event->lineStart, event->lineStart + kCacheLineSize - 1);
            DrawText(buffer, x, y, 18, kText);
            y += 28;
            DrawText(event->hit ? "Last access: HIT (+4 cycles)" : "Last access: MISS (+100 cycles)",
                     x,
                     y,
                     18,
                     event->hit ? kHitColor : kMissColor);
            y += 28;
            if (event->evictedLineStart)
            {
                std::snprintf(buffer, sizeof(buffer), "Evicted line: %02d-%02d", *event->evictedLineStart, *event->evictedLineStart + kCacheLineSize - 1);
                DrawText(buffer, x, y, 18, kEvictColor);
            }
        }
    }

    void drawHelp(const SimulationState& simulation) const
    {
        const Rectangle panel{820, 448, 430, 298};
        drawPanel(panel, "Controls");

        const int x = static_cast<int>(panel.x) + 24;
        int y = static_cast<int>(panel.y) + 58;
        DrawText("Space   pause / resume", x, y, 18, kText);
        y += 28;
        DrawText("R       reset current traversal", x, y, 18, kText);
        y += 28;
        DrawText("1       sequential array", x, y, 18, kText);
        y += 28;
        DrawText("2       random access", x, y, 18, kText);
        y += 28;
        DrawText("3       linked list pointer chasing", x, y, 18, kText);
        y += 28;
        DrawText("Up/Down adjust speed", x, y, 18, kText);
        y += 28;
        DrawText("Tab     toggle this overlay", x, y, 18, kText);
        y += 44;
        DrawText(simulation.paused ? "Paused" : "Running", x, y, 20, simulation.paused ? kEvictColor : kHitColor);
    }
};

void handleInput(SimulationState& simulation)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        simulation.paused = !simulation.paused;
    }
    if (IsKeyPressed(KEY_R))
    {
        simulation.reset();
    }
    if (IsKeyPressed(KEY_ONE))
    {
        simulation.setPattern(PatternKind::Sequential);
    }
    if (IsKeyPressed(KEY_TWO))
    {
        simulation.setPattern(PatternKind::Random);
    }
    if (IsKeyPressed(KEY_THREE))
    {
        simulation.setPattern(PatternKind::LinkedList);
    }
    if (IsKeyPressed(KEY_UP))
    {
        simulation.faster();
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        simulation.slower();
    }
    if (IsKeyPressed(KEY_TAB))
    {
        simulation.showOverlay = !simulation.showOverlay;
    }
    if (IsKeyPressed(KEY_ENTER) && simulation.paused)
    {
        simulation.stepOnce();
    }
}
}

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kWindowWidth, kWindowHeight, "memory-playground");
    SetTargetFPS(60);

    SimulationState simulation;
    Renderer renderer;

    while (!WindowShouldClose())
    {
        handleInput(simulation);
        simulation.update(GetFrameTime());
        renderer.draw(simulation);
    }

    CloseWindow();
    return 0;
}

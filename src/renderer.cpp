#include "renderer.hpp"

#include <algorithm>
#include <cstdio>

namespace memory_playground
{
namespace
{
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
}

void Renderer::draw(const SimulationState& simulation)
{
    const Layout layout = makeLayout(simulation);

    BeginDrawing();
    ClearBackground(kBackground);

    drawTitle(simulation);
    drawRegisters(simulation, layout.registers);
    drawCache(simulation, layout.cache);
    drawRam(simulation, layout.ram);
    drawFlow(simulation, layout);
    drawMetrics(simulation, layout.metrics);

    if (layout.showHelp)
    {
        drawHelp(simulation, layout.help);
    }

    EndDrawing();
}

Layout Renderer::makeLayout(const SimulationState& simulation)
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    constexpr float margin = 30.0f;
    constexpr float gap = 30.0f;
    constexpr float rightWidth = 430.0f;

    const float usableRightWidth = std::min(rightWidth, std::max(340.0f, width * 0.34f));
    const float rightX = width - margin - usableRightWidth;
    const float leftWidth = rightX - margin - gap;
    const float bottomY = 448.0f;
    const float bottomHeight = std::max(260.0f, height - bottomY - margin);

    Layout layout;
    layout.registers = Rectangle{margin, 74.0f, leftWidth, 116.0f};
    layout.cache = Rectangle{margin, 214.0f, leftWidth, 210.0f};
    layout.metrics = Rectangle{rightX, 74.0f, usableRightWidth, 350.0f};
    layout.showHelp = simulation.showOverlay;
    layout.help = Rectangle{rightX, bottomY, usableRightWidth, bottomHeight};
    layout.ram = Rectangle{
        margin,
        bottomY,
        layout.showHelp ? leftWidth : width - margin * 2.0f,
        bottomHeight
    };
    return layout;
}

FlowAnchors Renderer::makeFlowAnchors(const Layout& layout)
{
    return FlowAnchors{
        Vector2{layout.ram.x + layout.ram.width * 0.78f, layout.ram.y + layout.ram.height * 0.48f},
        Vector2{layout.cache.x + layout.cache.width * 0.88f, layout.cache.y + layout.cache.height * 0.55f},
        Vector2{layout.registers.x + layout.registers.width * 0.82f, layout.registers.y + layout.registers.height * 0.52f},
        Vector2{layout.cache.x + layout.cache.width - 330.0f, layout.cache.y + 12.0f}
    };
}

Color Renderer::fadeTo(Color base, Color highlight, float amount)
{
    amount = std::clamp(amount, 0.0f, 1.0f);
    return Color{
        static_cast<unsigned char>(base.r + (highlight.r - base.r) * amount),
        static_cast<unsigned char>(base.g + (highlight.g - base.g) * amount),
        static_cast<unsigned char>(base.b + (highlight.b - base.b) * amount),
        255
    };
}

Color Renderer::cacheFlashColor(CacheFlashKind kind)
{
    switch (kind)
    {
    case CacheFlashKind::Hit:
        return kHitColor;
    case CacheFlashKind::Miss:
        return kMissColor;
    case CacheFlashKind::Neutral:
        return kCacheColor;
    }
    return kCacheColor;
}

void Renderer::drawPanel(Rectangle bounds, const char* title) const
{
    DrawRectangleRounded(bounds, 0.06f, 8, kPanel);
    DrawRectangleRoundedLines(bounds, 0.06f, 8, kPanelBorder);
    DrawText(title, static_cast<int>(bounds.x) + 18, static_cast<int>(bounds.y) + 14, 22, kText);
}

void Renderer::drawTitle(const SimulationState& simulation) const
{
    DrawText("memory-playground", 30, 22, 28, kText);
    DrawText(simulation.getPattern().description(), 310, 30, 18, kMutedText);
}

void Renderer::drawRegisters(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Registers");

    const auto& slots = simulation.getRegisters().getSlots();
    const float gap = 14.0f;
    const float startX = panel.x + 160.0f;
    const float availableWidth = panel.width - 190.0f;
    const float slotW = std::max(86.0f, (availableWidth - gap * 3.0f) / static_cast<float>(slots.size()));
    for (int i = 0; i < static_cast<int>(slots.size()); ++i)
    {
        const float x = startX + i * (slotW + gap);
        const Rectangle slotRect{x, panel.y + 43, slotW, 48};
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

void Renderer::drawCache(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "L1 Cache (8 cache lines, FIFO eviction)");

    const auto& slots = simulation.getCache().getSlots();
    const auto& event = simulation.getLastEvent();
    const int columns = panel.width >= 680.0f ? 4 : 2;
    const float horizontalGap = 18.0f;
    const float slotW = (panel.width - 56.0f - horizontalGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    for (int i = 0; i < static_cast<int>(slots.size()); ++i)
    {
        const float x = panel.x + 28.0f + static_cast<float>(i % columns) * (slotW + horizontalGap);
        const float y = panel.y + 54.0f + static_cast<float>(i / columns) * 70.0f;
        const Rectangle slotRect{x, y, slotW, 50};

        Color fill = slots[i].valid ? kCacheColor : Color{45, 54, 65, 255};
        fill = fadeTo(fill, cacheFlashColor(slots[i].flashKind), slots[i].flash);
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

void Renderer::drawRam(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "RAM (128 cells grouped into 8-cell cache lines)");

    const auto& memory = simulation.getMemory();
    const auto& event = simulation.getLastEvent();
    const int lineCount = kRamCellCount / kCacheLineSize;
    const int columns = panel.width >= 900.0f ? 2 : 1;
    const int linesPerColumn = (lineCount + columns - 1) / columns;
    const float labelWidth = 72.0f;
    const float innerPad = 28.0f;
    const float columnGap = 22.0f;
    const float columnW = (panel.width - innerPad * 2.0f - columnGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float cellW = std::clamp((columnW - labelWidth - 14.0f) / static_cast<float>(kCacheLineSize), 28.0f, 43.0f);
    const float cellH = 23.0f;
    const float startX = panel.x + innerPad + labelWidth;
    const float startY = panel.y + 58.0f;

    for (int line = 0; line < lineCount; ++line)
    {
        const int column = line / linesPerColumn;
        const int row = line % linesPerColumn;
        const float xBase = startX + static_cast<float>(column) * (columnW + columnGap);
        const float y = startY + row * 28.0f;

        char lineLabel[32];
        std::snprintf(lineLabel, sizeof(lineLabel), "Line %02d", line);
        DrawText(lineLabel, static_cast<int>(xBase - labelWidth), static_cast<int>(y) + 4, 15, kMutedText);

        const Rectangle groupRect{xBase - 5.0f, y - 5.0f, kCacheLineSize * cellW + 10.0f, cellH + 10.0f};
        const bool activeLine = event && event->lineStart == line * kCacheLineSize;
        DrawRectangleRoundedLines(groupRect, 0.05f, 6, activeLine ? kHitColor : kLineColor);

        for (int offset = 0; offset < kCacheLineSize; ++offset)
        {
            const int address = line * kCacheLineSize + offset;
            const float x = xBase + offset * cellW;
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
            std::snprintf(label, sizeof(label), "%03d", memory[address].address);
            DrawText(label, static_cast<int>(x) + 5, static_cast<int>(y) + 5, 13, kText);
        }
    }

    if (event && event->address >= 0 && event->address < static_cast<int>(memory.size()))
    {
        char nextLabel[96];
        std::snprintf(nextLabel, sizeof(nextLabel), "current cell next pointer: %03d -> %03d", event->address, memory[event->address].nextAddress);
        DrawText(nextLabel, static_cast<int>(panel.x) + 38, static_cast<int>(panel.y + panel.height - 62), 17, kMutedText);
    }

    DrawText("A miss loads the whole highlighted RAM cache line into L1, not just one cell.",
             static_cast<int>(panel.x) + 38,
             static_cast<int>(panel.y + panel.height - 34),
             17,
             kMutedText);
}

void Renderer::drawFlow(const SimulationState& simulation, const Layout& layout) const
{
    const auto& event = simulation.getLastEvent();
    if (!event)
    {
        return;
    }

    const FlowAnchors anchors = makeFlowAnchors(layout);
    const float t = std::clamp(event->age / 0.7f, 0.0f, 1.0f);
    const Color color = event->hit ? kHitColor : kMissColor;

    Vector2 point = anchors.cache;
    if (t < 0.5f)
    {
        const float local = t / 0.5f;
        point = Vector2{
            anchors.ram.x + (anchors.cache.x - anchors.ram.x) * local,
            anchors.ram.y + (anchors.cache.y - anchors.ram.y) * local
        };
    }
    else
    {
        const float local = (t - 0.5f) / 0.5f;
        point = Vector2{
            anchors.cache.x + (anchors.registers.x - anchors.cache.x) * local,
            anchors.cache.y + (anchors.registers.y - anchors.cache.y) * local
        };
    }

    DrawLineEx(anchors.ram, anchors.cache, 3.0f, Fade(color, 0.45f));
    DrawLineEx(anchors.cache, anchors.registers, 3.0f, Fade(color, 0.45f));
    DrawCircleV(point, 10.0f, color);

    const char* label = event->hit ? "cache hit: L1 -> register" : "cache miss: RAM -> L1 -> register";
    const int fontSize = 18;
    const int textWidth = MeasureText(label, fontSize);
    const float labelWidth = static_cast<float>(textWidth + 24);
    const float minLabelX = layout.cache.x + 18.0f;
    const float maxLabelX = layout.cache.x + layout.cache.width - labelWidth - 18.0f;
    const Rectangle labelBox{
        std::clamp(anchors.label.x, minLabelX, std::max(minLabelX, maxLabelX)),
        anchors.label.y,
        labelWidth,
        32.0f
    };

    DrawRectangleRounded(labelBox, 0.25f, 8, Color{24, 30, 38, 238});
    DrawRectangleRoundedLines(labelBox, 0.25f, 8, Fade(color, 0.75f));
    DrawText(label,
             static_cast<int>(labelBox.x) + 12,
             static_cast<int>(labelBox.y) + 7,
             fontSize,
             color);
}

void Renderer::drawMetrics(const SimulationState& simulation, Rectangle panel) const
{
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
        std::snprintf(buffer, sizeof(buffer), "Current address: %03d", event->address);
        DrawText(buffer, x, y, 18, kText);
        y += 28;
        std::snprintf(buffer, sizeof(buffer), "Cache line: %03d-%03d", event->lineStart, event->lineStart + kCacheLineSize - 1);
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
            std::snprintf(buffer, sizeof(buffer), "Evicted line: %03d-%03d", *event->evictedLineStart, *event->evictedLineStart + kCacheLineSize - 1);
            DrawText(buffer, x, y, 18, kEvictColor);
        }
    }
}

void Renderer::drawHelp(const SimulationState& simulation, Rectangle panel) const
{
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
}

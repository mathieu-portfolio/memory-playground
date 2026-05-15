#include "renderer.hpp"
#include "rendering/render_style.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace memory_playground
{
using namespace rendering_detail;

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
        const Rectangle slotRect{x, panel.y + 39, slotW, 42};
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
        DrawText(label, static_cast<int>(x) + 13, static_cast<int>(slotRect.y) + 12, 16, kText);
    }
}

void Renderer::drawCache(const SimulationState& simulation, Rectangle panel) const
{
    char title[96];
    std::snprintf(title, sizeof(title), "L1 Cache (%d lines, FIFO eviction)", simulation.getSettings().cacheLineCount);
    drawPanel(panel, title);

    const auto& slots = simulation.getCache().getSlots();
    const auto& event = simulation.getLastEvent();
    const int columns = panel.width >= 680.0f ? 4 : 2;
    const float horizontalGap = 12.0f;
    const float verticalGap = 10.0f;
    const int rows = (static_cast<int>(slots.size()) + columns - 1) / columns;
    const float slotW = (panel.width - 2.0f * kPanelPad - horizontalGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float slotH = std::max(20.0f, (panel.height - kHeaderHeight - 18.0f - verticalGap * static_cast<float>(rows - 1)) / static_cast<float>(rows));
    for (int i = 0; i < static_cast<int>(slots.size()); ++i)
    {
        const float x = panel.x + kPanelPad + static_cast<float>(i % columns) * (slotW + horizontalGap);
        const float y = panel.y + kHeaderHeight + static_cast<float>(i / columns) * (slotH + verticalGap);
        const Rectangle slotRect{x, y, slotW, slotH};

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
            std::snprintf(label, sizeof(label), "L%d  %02d-%02d", i, slots[i].lineStart, slots[i].lineStart + simulation.getSettings().cacheLineSize - 1);
        }
        else
        {
            std::snprintf(label, sizeof(label), "L%d  empty", i);
        }
        DrawText(label, static_cast<int>(x) + 9, static_cast<int>(y + slotH * 0.5f - 7.0f), 14, kText);
    }
}

void Renderer::drawRam(const SimulationState& simulation, Rectangle panel) const
{
    char title[96];
    std::snprintf(title, sizeof(title), "RAM (128 cells grouped into %d-cell cache lines)", simulation.getSettings().cacheLineSize);
    drawPanel(panel, title);

    const auto& memory = simulation.getMemory();
    const auto& event = simulation.getLastEvent();
    const int lineSize = simulation.getSettings().cacheLineSize;
    const int lineCount = (kRamCellCount + lineSize - 1) / lineSize;
    const int columns = panel.width >= 900.0f ? 2 : 1;
    const int linesPerColumn = (lineCount + columns - 1) / columns;
    const float labelWidth = 72.0f;
    const float innerPad = 28.0f;
    const float columnGap = 22.0f;
    const float columnW = (panel.width - innerPad * 2.0f - columnGap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    const float cellW = std::clamp((columnW - labelWidth - 14.0f) / static_cast<float>(lineSize), 12.0f, 43.0f);
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

        const Rectangle groupRect{xBase - 5.0f, y - 5.0f, lineSize * cellW + 10.0f, cellH + 10.0f};
            const bool activeLine = event && event->lineStart == line * lineSize;
            DrawRectangleRoundedLines(groupRect, 0.05f, 6, activeLine ? kHitColor : kLineColor);

        for (int offset = 0; offset < lineSize; ++offset)
        {
            const int address = line * lineSize + offset;
            if (address >= static_cast<int>(memory.size()))
            {
                continue;
            }
            const float x = xBase + offset * cellW;
            const Rectangle cellRect{x, y, cellW - 3.0f, cellH};

            Color fill = kRamColor;
            if (event && event->address == address)
            {
                fill = event->hit ? kHitColor : kMissColor;
            }
            else if (event && event->lineStart == line * lineSize)
            {
                fill = fadeTo(fill, event->hit ? kHitColor : kMissColor, 0.28f);
            }
            else if (event && event->evictedLineStart && *event->evictedLineStart == line * lineSize)
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
        std::snprintf(nextLabel, sizeof(nextLabel), "Current address %03d   line %03d-%03d   linked-list next %03d",
                      event->address,
                      event->lineStart,
                      event->lineStart + lineSize - 1,
                      memory[event->address].nextAddress);
        DrawText(nextLabel, static_cast<int>(panel.x + kPanelPad), static_cast<int>(panel.y + panel.height - 58), 15, kMutedText);
    }

    DrawText("Cache lines are groups: loading one address also brings its neighbors.",
             static_cast<int>(panel.x + kPanelPad),
             static_cast<int>(panel.y + panel.height - 32),
             15,
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
    if (event->hit)
    {
        point = pointOnCurve(anchors.cache, anchors.registers, t);
    }
    else if (t < 0.5f)
    {
        const float local = t / 0.5f;
        point = pointOnCurve(anchors.ram, anchors.cache, local);
    }
    else
    {
        const float local = (t - 0.5f) / 0.5f;
        point = pointOnCurve(anchors.cache, anchors.registers, local);
    }

    if (!event->hit)
    {
        drawCurve(anchors.ram, anchors.cache, Fade(color, 0.22f));
    }
    drawCurve(anchors.cache, anchors.registers, Fade(color, 0.28f));
    for (int i = 3; i >= 1; --i)
    {
        DrawCircleV(point, 4.0f + static_cast<float>(i) * 4.0f, Fade(color, 0.08f * static_cast<float>(i)));
    }
    DrawCircleV(point, 6.0f, color);

    const char* label = event->hit ? "Hit: L1 -> register" : "Miss: RAM -> L1 -> register";
    const int fontSize = 14;
    const int textWidth = MeasureText(label, fontSize);
    const float labelWidth = static_cast<float>(textWidth + 20);
    const float minLabelX = layout.cache.x + 18.0f;
    const float maxLabelX = layout.cache.x + layout.cache.width - labelWidth - 18.0f;
    const Rectangle labelBox{
        std::clamp(anchors.label.x, minLabelX, std::max(minLabelX, maxLabelX)),
        anchors.label.y,
        labelWidth,
        26.0f
    };

    DrawRectangleRounded(labelBox, 0.25f, 8, Color{24, 30, 38, 226});
    DrawRectangleRoundedLines(labelBox, 0.25f, 8, Fade(color, 0.75f));
    DrawText(label,
             static_cast<int>(labelBox.x) + 10,
             static_cast<int>(labelBox.y) + 6,
             fontSize,
             color);
}
}

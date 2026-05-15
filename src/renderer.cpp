#include "renderer.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace memory_playground
{
namespace
{
constexpr Color kBackground{18, 22, 28, 255};
constexpr Color kPanel{29, 35, 44, 255};
constexpr Color kPanelBorder{56, 66, 78, 255};
constexpr Color kText{229, 236, 244, 255};
constexpr Color kMutedText{151, 163, 178, 255};
constexpr Color kRamColor{61, 84, 111, 255};
constexpr Color kCacheColor{49, 119, 99, 255};
constexpr Color kRegisterColor{112, 91, 154, 255};
constexpr Color kHitColor{88, 196, 122, 255};
constexpr Color kMissColor{222, 101, 101, 255};
constexpr Color kEvictColor{226, 177, 76, 255};
constexpr Color kLineColor{101, 116, 139, 255};
constexpr float kPanelPad = 18.0f;
constexpr float kHeaderHeight = 42.0f;

float normalized(float value, float minValue, float maxValue)
{
    return std::clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
}

void drawSlider(Rectangle track, float amount, Color color)
{
    DrawRectangleRounded(track, 0.5f, 8, Color{45, 53, 64, 255});
    DrawRectangleRounded(Rectangle{track.x, track.y, track.width * amount, track.height}, 0.5f, 8, Fade(color, 0.85f));
    DrawCircle(static_cast<int>(track.x + track.width * amount), static_cast<int>(track.y + track.height * 0.5f), 7.0f, color);
}

Vector2 quadraticPoint(Vector2 a, Vector2 control, Vector2 b, float t)
{
    const float inv = 1.0f - t;
    return Vector2{
        inv * inv * a.x + 2.0f * inv * t * control.x + t * t * b.x,
        inv * inv * a.y + 2.0f * inv * t * control.y + t * t * b.y
    };
}

void drawCurve(Vector2 a, Vector2 b, Color color)
{
    const Vector2 control{(a.x + b.x) * 0.5f + 26.0f, (a.y + b.y) * 0.5f - 42.0f};
    Vector2 previous = a;
    for (int i = 1; i <= 20; ++i)
    {
        const float t = static_cast<float>(i) / 20.0f;
        const Vector2 point = quadraticPoint(a, control, b, t);
        DrawLineEx(previous, point, 2.0f, color);
        previous = point;
    }
}

Vector2 pointOnCurve(Vector2 a, Vector2 b, float t)
{
    const Vector2 control{(a.x + b.x) * 0.5f + 26.0f, (a.y + b.y) * 0.5f - 42.0f};
    return quadraticPoint(a, control, b, t);
}
}

void Renderer::draw(const SimulationState& simulation)
{
    const Layout layout = makeLayout();

    BeginDrawing();
    ClearBackground(kBackground);

    drawTitle(simulation);
    drawRegisters(simulation, layout.registers);
    drawCache(simulation, layout.cache);
    drawTimeline(simulation, layout.timeline);
    drawRam(simulation, layout.ram);
    drawFlow(simulation, layout);
    drawMetrics(simulation, layout.metrics);
    drawPerformanceGraph(simulation, layout.graph);
    drawSettings(simulation, layout.settings);
    drawChallenges(simulation, layout.challenges);
    drawLearningFeedback(simulation, layout.learn);
    drawFooter(simulation, layout.footer);

    EndDrawing();
}

Layout Renderer::makeLayout()
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    constexpr float margin = 30.0f;
    constexpr float gap = 30.0f;
    constexpr float rightWidth = 390.0f;
    constexpr float footerHeight = 44.0f;

    const float usableRightWidth = std::min(rightWidth, std::max(340.0f, width * 0.30f));
    const float rightX = width - margin - usableRightWidth;
    const float leftWidth = rightX - margin - gap;
    const float bottomY = 458.0f;
    const float bottomHeight = std::max(260.0f, height - bottomY - footerHeight - margin - 14.0f);
    const float rightBottom = height - footerHeight - margin - 14.0f;

    Layout layout;
    layout.registers = Rectangle{margin, 74.0f, leftWidth, 96.0f};
    layout.cache = Rectangle{margin, 190.0f, leftWidth, 160.0f};
    layout.timeline = Rectangle{margin, 370.0f, leftWidth, 68.0f};
    layout.metrics = Rectangle{rightX, 74.0f, usableRightWidth, 104.0f};
    layout.graph = Rectangle{rightX, 194.0f, usableRightWidth, 104.0f};
    layout.settings = Rectangle{rightX, 314.0f, usableRightWidth, std::clamp((rightBottom - 314.0f) * 0.34f, 190.0f, 220.0f)};
    layout.challenges = Rectangle{rightX, layout.settings.y + layout.settings.height + 16.0f, usableRightWidth, 176.0f};
    layout.learn = Rectangle{rightX, layout.challenges.y + layout.challenges.height + 16.0f, usableRightWidth, std::max(112.0f, rightBottom - (layout.challenges.y + layout.challenges.height + 16.0f))};
    layout.footer = Rectangle{margin, height - footerHeight - margin, width - margin * 2.0f, footerHeight};
    layout.ram = Rectangle{
        margin,
        bottomY,
        leftWidth,
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
    DrawText(title, static_cast<int>(bounds.x + kPanelPad), static_cast<int>(bounds.y + 13.0f), 18, kText);
}

void Renderer::drawTitle(const SimulationState& simulation) const
{
    DrawText("memory-playground", 30, 22, 28, kText);
    DrawText(simulation.getPattern().description(), 310, 31, 16, kMutedText);
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

void Renderer::drawMetrics(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Metrics");

    const Metrics& metrics = simulation.getMetrics();

    char buffer[160];
    const float colW = (panel.width - 2.0f * kPanelPad) / 3.0f;
    const float y = panel.y + kHeaderHeight + 4.0f;
    const float labelY = y + 34.0f;

    std::snprintf(buffer, sizeof(buffer), "%d", metrics.totalAccesses);
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad), static_cast<int>(y), 24, kText);
    DrawText("accesses", static_cast<int>(panel.x + kPanelPad), static_cast<int>(labelY), 13, kMutedText);

    std::snprintf(buffer, sizeof(buffer), "%.0f%%", metrics.hitRate());
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad + colW), static_cast<int>(y), 24, kHitColor);
    DrawText("hit rate", static_cast<int>(panel.x + kPanelPad + colW), static_cast<int>(labelY), 13, kMutedText);

    std::snprintf(buffer, sizeof(buffer), "%.1f", metrics.averageCycles());
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad + colW * 2.0f), static_cast<int>(y), 24, kText);
    DrawText("cycles/access", static_cast<int>(panel.x + kPanelPad + colW * 2.0f), static_cast<int>(labelY), 13, kMutedText);
}

void Renderer::drawTimeline(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Access Timeline");

    const auto& entries = simulation.getAccessHistory().getEntries();
    const float x = panel.x + kPanelPad;
    const float y = panel.y + 46.0f;
    const float available = panel.width - 2.0f * kPanelPad;
    const float gap = 3.0f;
    const float itemW = std::max(6.0f, (available - gap * static_cast<float>(kAccessHistorySize - 1)) / static_cast<float>(kAccessHistorySize));

    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        const auto& entry = entries[static_cast<std::size_t>(i)];
        const bool current = i == static_cast<int>(entries.size()) - 1;
        const Rectangle marker{x + static_cast<float>(i) * (itemW + gap), y, itemW, current ? 15.0f : 10.0f};
        DrawRectangleRounded(marker, 0.35f, 6, entry.hit ? kHitColor : kMissColor);
        if (entry.evicted)
        {
            DrawCircle(static_cast<int>(marker.x + marker.width * 0.5f), static_cast<int>(marker.y - 4.0f), 3.0f, kEvictColor);
        }
        if (current)
        {
            DrawRectangleRoundedLines(Rectangle{marker.x - 2.0f, marker.y - 2.0f, marker.width + 4.0f, marker.height + 4.0f}, 0.35f, 6, WHITE);
        }
    }

    DrawText("hit", static_cast<int>(panel.x + panel.width - 100.0f), static_cast<int>(panel.y + 17.0f), 13, kHitColor);
    DrawText("miss", static_cast<int>(panel.x + panel.width - 60.0f), static_cast<int>(panel.y + 17.0f), 13, kMissColor);
}

void Renderer::drawPerformanceGraph(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Recent Hit Rate");

    const auto& samples = simulation.getPerformanceHistory().getSamples();
    const Rectangle plot{panel.x + kPanelPad, panel.y + 50.0f, panel.width - 2.0f * kPanelPad, panel.height - 68.0f};
    DrawRectangleRounded(plot, 0.04f, 6, Color{24, 30, 38, 255});
    DrawText("100%", static_cast<int>(plot.x), static_cast<int>(plot.y - 18.0f), 12, kMutedText);
    DrawText("0%", static_cast<int>(plot.x), static_cast<int>(plot.y + plot.height + 4.0f), 12, kMutedText);
    DrawLine(static_cast<int>(plot.x), static_cast<int>(plot.y + plot.height * 0.5f), static_cast<int>(plot.x + plot.width), static_cast<int>(plot.y + plot.height * 0.5f), Fade(kLineColor, 0.45f));

    if (samples.size() >= 2)
    {
        for (int i = 1; i < static_cast<int>(samples.size()); ++i)
        {
            const float prevX = plot.x + plot.width * static_cast<float>(i - 1) / static_cast<float>(kPerformanceHistorySize - 1);
            const float nextX = plot.x + plot.width * static_cast<float>(i) / static_cast<float>(kPerformanceHistorySize - 1);
            const float prevY = plot.y + plot.height * (1.0f - samples[static_cast<std::size_t>(i - 1)].hitRate / 100.0f);
            const float nextY = plot.y + plot.height * (1.0f - samples[static_cast<std::size_t>(i)].hitRate / 100.0f);
            DrawLineEx(Vector2{prevX, prevY}, Vector2{nextX, nextY}, 2.0f, kHitColor);
        }
    }

    char label[96];
    std::snprintf(label, sizeof(label), "%.1f cycles/access", simulation.getMetrics().averageCycles());
    DrawText(label, static_cast<int>(panel.x + panel.width - 146.0f), static_cast<int>(panel.y + 18.0f), 13, kMutedText);
}

void Renderer::drawSettings(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Experiment");

    const auto& settings = simulation.getSettings();
    char line[128];
    const float labelX = panel.x + kPanelPad;
    const float trackX = panel.x + 146.0f;
    const float valueX = panel.x + panel.width - 54.0f;
    const float trackW = panel.width - 176.0f;
    const char* labels[] = {"Speed", "Line size", "L1 lines", "Hit", "Miss"};
    const char* hints[] = {"Up/Down", "[ / ]", "- / =", "H / J", "N / M"};
    const float amounts[] = {
        normalized(settings.accessesPerSecond, 0.5f, 12.0f),
        normalized(static_cast<float>(settings.cacheLineSize), static_cast<float>(kMinCacheLineSize), static_cast<float>(kMaxCacheLineSize)),
        normalized(static_cast<float>(settings.cacheLineCount), static_cast<float>(kMinCacheLineCount), static_cast<float>(kMaxCacheLineCount)),
        normalized(static_cast<float>(settings.hitCycles), static_cast<float>(kMinHitCycles), static_cast<float>(kMaxHitCycles)),
        normalized(static_cast<float>(settings.missCycles), static_cast<float>(kMinMissCycles), static_cast<float>(kMaxMissCycles))
    };
    const Color colors[] = {kText, kCacheColor, kCacheColor, kHitColor, kMissColor};

    for (int i = 0; i < 5; ++i)
    {
        const float y = panel.y + 52.0f + static_cast<float>(i) * 30.0f;
        DrawText(labels[i], static_cast<int>(labelX), static_cast<int>(y - 6.0f), 14, kText);
        DrawText(hints[i], static_cast<int>(labelX), static_cast<int>(y + 9.0f), 11, kMutedText);
        drawSlider(Rectangle{trackX, y, trackW, 8.0f}, amounts[i], colors[i]);

        if (i == 0)
        {
            std::snprintf(line, sizeof(line), "%.1f", settings.accessesPerSecond);
        }
        else if (i == 1)
        {
            std::snprintf(line, sizeof(line), "%d", settings.cacheLineSize);
        }
        else if (i == 2)
        {
            std::snprintf(line, sizeof(line), "%d", settings.cacheLineCount);
        }
        else if (i == 3)
        {
            std::snprintf(line, sizeof(line), "%d", settings.hitCycles);
        }
        else
        {
            std::snprintf(line, sizeof(line), "%d", settings.missCycles);
        }
        DrawText(line, static_cast<int>(valueX), static_cast<int>(y - 6.0f), 14, colors[i]);
    }
}

void Renderer::drawChallenges(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Challenges");

    const auto& challenges = simulation.getChallenges().getChallenges();
    const float cardGap = 8.0f;
    const float cardH = (panel.height - kHeaderHeight - cardGap * 3.0f - 10.0f) / 4.0f;
    for (int i = 0; i < static_cast<int>(challenges.size()); ++i)
    {
        const Challenge& challenge = challenges[static_cast<std::size_t>(i)];
        Color stateColor = kMutedText;
        const char* stateLabel = "Active";
        if (challenge.state == ChallengeState::Completed)
        {
            stateColor = kHitColor;
            stateLabel = "Complete";
        }
        else if (challenge.state == ChallengeState::Failed)
        {
            stateColor = kMissColor;
            stateLabel = "Failed";
        }

        const Rectangle card{
            panel.x + kPanelPad,
            panel.y + kHeaderHeight + static_cast<float>(i) * (cardH + cardGap),
            panel.width - 2.0f * kPanelPad,
            cardH
        };
        DrawRectangleRounded(card, 0.08f, 8, Color{24, 30, 38, 255});
        DrawRectangleRoundedLines(card, 0.08f, 8, Fade(stateColor, 0.35f));
        DrawText(challenge.title.c_str(), static_cast<int>(card.x + 10.0f), static_cast<int>(card.y + 7.0f), 14, kText);
        DrawText(stateLabel, static_cast<int>(card.x + card.width - 78.0f), static_cast<int>(card.y + 7.0f), 13, stateColor);

        const Rectangle bar{card.x + 10.0f, card.y + card.height - 11.0f, card.width - 20.0f, 5.0f};
        DrawRectangleRounded(bar, 0.6f, 6, Color{45, 53, 64, 255});
        DrawRectangleRounded(Rectangle{bar.x, bar.y, bar.width * std::clamp(challenge.progressAmount, 0.0f, 1.0f), bar.height}, 0.6f, 6, Fade(stateColor, 0.9f));
        DrawText(challenge.progress.c_str(), static_cast<int>(card.x + 10.0f), static_cast<int>(card.y + 24.0f), 12, kMutedText);
    }
}

void Renderer::drawLearningFeedback(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Learn");

    const char* concept = "Spatial locality";
    const char* modeText = "Nearby array elements reuse cache lines.";
    const char* detailText = "One miss can make the next cells cheap.";
    const std::string modeName = simulation.getPattern().name();
    if (modeName.find("Random") != std::string::npos)
    {
        concept = "Poor locality";
        modeText = "Random jumps rarely reuse loaded lines.";
        detailText = "The cache cannot build a rhythm.";
    }
    else if (modeName.find("Linked") != std::string::npos)
    {
        concept = "Pointer chasing";
        modeText = "Each node reveals the next address late.";
        detailText = "The CPU cannot easily look ahead.";
    }

    const Rectangle conceptCard{panel.x + kPanelPad, panel.y + kHeaderHeight, panel.width - 2.0f * kPanelPad, 58.0f};
    DrawRectangleRounded(conceptCard, 0.08f, 8, Color{24, 30, 38, 255});
    DrawText(concept, static_cast<int>(conceptCard.x + 12.0f), static_cast<int>(conceptCard.y + 8.0f), 15, kText);
    DrawText(modeText, static_cast<int>(conceptCard.x + 12.0f), static_cast<int>(conceptCard.y + 30.0f), 13, kMutedText);

    const auto& event = simulation.getLastEvent();
    const Rectangle eventCard{panel.x + kPanelPad, conceptCard.y + conceptCard.height + 10.0f, panel.width - 2.0f * kPanelPad, 58.0f};
    DrawRectangleRounded(eventCard, 0.08f, 8, Color{24, 30, 38, 255});
    if (!event)
    {
        DrawText("Last access", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kText);
        DrawText("Waiting for data movement.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
        return;
    }

    if (event->hit)
    {
        DrawText("Hit", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kHitColor);
        DrawText("Reused a line already in L1.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
    }
    else
    {
        DrawText("Miss", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kMissColor);
        DrawText("Loaded a full line from RAM.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
    }

    if (event->evictedLineStart)
    {
        DrawText("Eviction", static_cast<int>(eventCard.x + eventCard.width - 76.0f), static_cast<int>(eventCard.y + 8.0f), 13, kEvictColor);
    }

    DrawText(detailText, static_cast<int>(panel.x + kPanelPad), static_cast<int>(eventCard.y + eventCard.height + 12.0f), 13, kMutedText);
}

void Renderer::drawFooter(const SimulationState& simulation, Rectangle panel) const
{
    DrawRectangleRounded(panel, 0.14f, 10, Color{24, 30, 38, 240});
    DrawRectangleRoundedLines(panel, 0.14f, 10, kPanelBorder);

    const char* runState = simulation.paused ? "Paused" : "Running";
    DrawText(runState,
             static_cast<int>(panel.x + 18.0f),
             static_cast<int>(panel.y + 13.0f),
             16,
             simulation.paused ? kEvictColor : kHitColor);

    const char* controls = "Space pause  R reset  1/2/3 modes  Enter step";
    DrawText(controls, static_cast<int>(panel.x + 110.0f), static_cast<int>(panel.y + 14.0f), 14, kMutedText);

    const char* settings = "Drag sliders or use Up/Down  [ ]  -/=  H/J  N/M";
    const int settingsX = static_cast<int>(std::max(panel.x + 520.0f, panel.x + panel.width - static_cast<float>(MeasureText(settings, 14)) - 18.0f));
    if (settingsX + MeasureText(settings, 14) < static_cast<int>(panel.x + panel.width - 8.0f))
    {
        DrawText(settings, settingsX, static_cast<int>(panel.y + 14.0f), 14, kMutedText);
    }
}
}

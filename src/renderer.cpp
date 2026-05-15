#include "renderer.hpp"
#include "rendering/render_style.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

namespace memory_playground
{
using namespace rendering_detail;

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

    const float usableRightWidth = std::min(kRightColumnWidth, std::max(360.0f, width * 0.29f));
    const float rightX = width - kOuterMargin - usableRightWidth;
    const float leftWidth = rightX - kOuterMargin - kColumnGap;

    const float footerY = height - kFooterHeight - kOuterMargin;
    const float contentBottom = footerY - kPanelGap;

    Layout layout;

    float leftY = kLeftTopY;
    layout.registers = Rectangle{kOuterMargin, leftY, leftWidth, kRegistersHeight};
    leftY += layout.registers.height + 20.0f;

    layout.cache = Rectangle{kOuterMargin, leftY, leftWidth, kCacheHeight};
    leftY += layout.cache.height + 16.0f;

    layout.timeline = Rectangle{kOuterMargin, leftY, leftWidth, kTimelineHeight};
    leftY += layout.timeline.height + 14.0f;

    layout.ram = Rectangle{
        kOuterMargin,
        leftY,
        leftWidth,
        std::max(300.0f, contentBottom - leftY)
    };

    float rightY = kLeftTopY;
    layout.metrics = Rectangle{rightX, rightY, usableRightWidth, kMetricsHeight};
    rightY += layout.metrics.height + kPanelGap;

    layout.graph = Rectangle{rightX, rightY, usableRightWidth, kGraphHeight};
    rightY += layout.graph.height + kPanelGap;

    layout.settings = Rectangle{rightX, rightY, usableRightWidth, kSettingsHeight};
    rightY += layout.settings.height + kPanelGap;

    // The challenge panel is sized from the challenge row model instead of being squeezed
    // into whatever height remains. This keeps title, progress text, and bar regions stable.
    layout.challenges = Rectangle{rightX, rightY, usableRightWidth, challengesHeightForCount(kUiDefaultChallengeCount)};
    rightY += layout.challenges.height + kPanelGap;

    layout.learn = Rectangle{rightX, rightY, usableRightWidth, std::max(kLearnHeight, contentBottom - rightY)};

    layout.footer = Rectangle{kOuterMargin, footerY, width - kOuterMargin * 2.0f, kFooterHeight};
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
}

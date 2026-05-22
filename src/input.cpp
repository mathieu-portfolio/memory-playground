#include "input.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>

namespace memory_playground
{
namespace
{
Rectangle makeSettingsPanel()
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    constexpr float margin = 30.0f;
    constexpr float rightWidth = 390.0f;
    constexpr float footerHeight = 44.0f;

    const float usableRightWidth = std::min(rightWidth, std::max(340.0f, width * 0.30f));
    const float rightX = width - margin - usableRightWidth;
    const float settingsY = 314.0f;
    const float availableHeight = height - footerHeight - margin - settingsY;
    return Rectangle{rightX, settingsY, usableRightWidth, std::clamp(availableHeight * 0.34f, 190.0f, 220.0f)};
}

float normalizedFromMouse(Rectangle track)
{
    const float mouseX = GetMousePosition().x;
    return std::clamp((mouseX - track.x) / track.width, 0.0f, 1.0f);
}

int valueFromMouse(Rectangle track, int minValue, int maxValue)
{
    return minValue + static_cast<int>(std::round(normalizedFromMouse(track) * static_cast<float>(maxValue - minValue)));
}

float speedFromMouse(Rectangle track)
{
    return 0.5f + normalizedFromMouse(track) * 11.5f;
}

Rectangle sliderTrack(Rectangle settingsPanel, int index)
{
    return Rectangle{settingsPanel.x + 146.0f, settingsPanel.y + 54.0f + static_cast<float>(index) * 30.0f, settingsPanel.width - 176.0f, 8.0f};
}

void handleMouseSliders(SimulationState& simulation)
{
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        return;
    }

    const Rectangle panel = makeSettingsPanel();
    const Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 5; ++i)
    {
        const Rectangle track = sliderTrack(panel, i);
        const Rectangle hitBox{track.x - 8.0f, track.y - 10.0f, track.width + 16.0f, 28.0f};
        if (!CheckCollisionPointRec(mouse, hitBox))
        {
            continue;
        }

        switch (i)
        {
        case 0:
            simulation.setSpeed(speedFromMouse(track));
            break;
        case 1:
            simulation.setCacheLineSize(valueFromMouse(track, kMinCacheLineSize, kMaxCacheLineSize));
            break;
        case 2:
            simulation.setCacheLineCount(valueFromMouse(track, kMinCacheLineCount, kMaxCacheLineCount));
            break;
        case 3:
            simulation.setHitCycles(valueFromMouse(track, kMinHitCycles, kMaxHitCycles));
            break;
        case 4:
            simulation.setMissCycles(valueFromMouse(track, kMinMissCycles, kMaxMissCycles));
            break;
        }
        return;
    }
}
}

void handleInput(SimulationState& simulation)
{
    handleMouseSliders(simulation);

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
    if (IsKeyPressed(KEY_LEFT_BRACKET))
    {
        simulation.adjustCacheLineSize(-1);
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET))
    {
        simulation.adjustCacheLineSize(1);
    }
    if (IsKeyPressed(KEY_MINUS))
    {
        simulation.adjustCacheLineCount(-1);
    }
    if (IsKeyPressed(KEY_EQUAL))
    {
        simulation.adjustCacheLineCount(1);
    }
    if (IsKeyPressed(KEY_H))
    {
        simulation.adjustHitCycles(-1);
    }
    if (IsKeyPressed(KEY_J))
    {
        simulation.adjustHitCycles(1);
    }
    if (IsKeyPressed(KEY_N))
    {
        simulation.adjustMissCycles(-10);
    }
    if (IsKeyPressed(KEY_M))
    {
        simulation.adjustMissCycles(10);
    }
    if (IsKeyPressed(KEY_B))
    {
        simulation.runBenchmarks();
    }
    if (IsKeyPressed(KEY_E))
    {
        simulation.exportLatestBenchmarkCsv();
    }
    if (IsKeyPressed(KEY_ENTER) && simulation.paused)
    {
        simulation.stepOnce();
    }
}
}

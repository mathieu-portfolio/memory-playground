#include "input.hpp"

#include "raylib.h"

namespace memory_playground
{
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

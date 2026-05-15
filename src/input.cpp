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

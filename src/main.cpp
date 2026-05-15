#include "app_config.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "simulation.hpp"

#include "raylib.h"

int main()
{
    using namespace memory_playground;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kWindowWidth, kWindowHeight, "memory-playground");
    SetWindowMinSize(kMinWindowWidth, kMinWindowHeight);
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

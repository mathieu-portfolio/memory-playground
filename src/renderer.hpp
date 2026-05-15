#pragma once

#include "simulation.hpp"

#include "raylib.h"

namespace memory_playground
{
struct Layout
{
    Rectangle registers{};
    Rectangle cache{};
    Rectangle metrics{};
    Rectangle ram{};
    Rectangle help{};
    bool showHelp = true;
};

struct FlowAnchors
{
    Vector2 ram{};
    Vector2 cache{};
    Vector2 registers{};
    Vector2 label{};
};

class Renderer
{
public:
    void draw(const SimulationState& simulation);

private:
    static Layout makeLayout(const SimulationState& simulation);
    static FlowAnchors makeFlowAnchors(const Layout& layout);
    static Color fadeTo(Color base, Color highlight, float amount);
    static Color cacheFlashColor(CacheFlashKind kind);

    void drawPanel(Rectangle bounds, const char* title) const;
    void drawTitle(const SimulationState& simulation) const;
    void drawRegisters(const SimulationState& simulation, Rectangle panel) const;
    void drawCache(const SimulationState& simulation, Rectangle panel) const;
    void drawRam(const SimulationState& simulation, Rectangle panel) const;
    void drawFlow(const SimulationState& simulation, const Layout& layout) const;
    void drawMetrics(const SimulationState& simulation, Rectangle panel) const;
    void drawHelp(const SimulationState& simulation, Rectangle panel) const;
};
}

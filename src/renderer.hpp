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
    Rectangle graph{};
    Rectangle settings{};
    Rectangle challenges{};
    Rectangle learn{};
    Rectangle timeline{};
    Rectangle ram{};
    Rectangle footer{};
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
    static Layout makeLayout();
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
    void drawTimeline(const SimulationState& simulation, Rectangle panel) const;
    void drawPerformanceGraph(const SimulationState& simulation, Rectangle panel) const;
    void drawSettings(const SimulationState& simulation, Rectangle panel) const;
    void drawChallenges(const SimulationState& simulation, Rectangle panel) const;
    void drawLearningFeedback(const SimulationState& simulation, Rectangle panel) const;
    void drawBenchmarkReport(const SimulationState& simulation, Rectangle panel, float startY) const;
    void drawFooter(const SimulationState& simulation, Rectangle panel) const;
};
}

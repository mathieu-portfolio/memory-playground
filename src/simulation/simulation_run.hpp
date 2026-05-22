
#pragma once

#include "simulation/instrumentation.hpp"
#include "simulation/scenario_definition.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace memory_playground
{
struct SimulationRun
{
    std::string scenarioName;
    unsigned int seed = 0;

    std::vector<TraceEvent> traceEvents;
    MetricsSnapshot metrics;

    double runDurationMs = 0.0;

    bool exportCsv(const std::string& path) const
    {
        std::ofstream file(path);

        if (!file.is_open())
        {
            return false;
        }

        file << "tick,type,address,line_start,line_end,cycles\n";

        for (const TraceEvent& event : traceEvents)
        {
            file << event.tick << ","
                 << static_cast<int>(event.type) << ","
                 << event.address << ","
                 << event.lineStart << ","
                 << event.lineEnd << ","
                 << event.simulatedCycles << "\n";
        }

        return true;
    }
};
}

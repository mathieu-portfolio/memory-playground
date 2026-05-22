#pragma once

#include "simulation/instrumentation.hpp"
#include "simulation/scenario_definition.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

namespace memory_playground
{
struct SimulationRun
{
    std::string scenarioName;
    std::uint32_t seed = 0;
    ExperimentSettings settings{};
    std::vector<TraceEvent> trace;
    MetricsSnapshot finalMetrics{};
    double durationMilliseconds = 0.0;
};

inline std::string simulationRunToCsv(const SimulationRun& run)
{
    std::ostringstream out;
    out << "tick,type,address,cache_line_start,cache_line_end,cache_slot,register_slot,evicted_line_start,cycles,label\n";
    for (const TraceEvent& event : run.trace)
    {
        out << event.tick << ','
            << traceEventTypeName(event.type) << ','
            << event.address << ','
            << event.cacheLineStart << ','
            << event.cacheLineEnd << ','
            << event.cacheSlot << ','
            << event.registerSlot << ',';
        if (event.evictedLineStart.has_value())
        {
            out << *event.evictedLineStart;
        }
        out << ',' << event.simulatedCycles << ',' << event.label << '\n';
    }
    return out.str();
}
}

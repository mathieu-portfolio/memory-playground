#pragma once

#include "simulation/constants.hpp"
#include "simulation/experiment_settings.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace memory_playground
{
enum class ScenarioPatternType
{
    Sequential,
    Random,
    Stride,
    CacheThrash,
    RepeatedHotSet,
    StructOfArrays,
    ArrayOfStructs
};

inline const char* scenarioPatternTypeName(ScenarioPatternType type)
{
    switch (type)
    {
    case ScenarioPatternType::Sequential: return "sequential";
    case ScenarioPatternType::Random: return "random";
    case ScenarioPatternType::Stride: return "stride";
    case ScenarioPatternType::CacheThrash: return "cache_thrash";
    case ScenarioPatternType::RepeatedHotSet: return "repeated_hot_set";
    case ScenarioPatternType::StructOfArrays: return "struct_of_arrays";
    case ScenarioPatternType::ArrayOfStructs: return "array_of_structs";
    }
    return "unknown";
}

struct ScenarioDefinition
{
    std::string name;
    std::string description;
    std::uint32_t seed = 7;
    int steps = 128;
    ScenarioPatternType pattern = ScenarioPatternType::Sequential;
    int addressStart = 0;
    int addressRange = kRamCellCount;
    int stride = 1;
    int hotSetSize = kDefaultCacheLineSize;
    float writeRatio = 0.0f;
    ExperimentSettings settings{};

    int clampedAddressStart() const
    {
        return std::clamp(addressStart, 0, kRamCellCount - 1);
    }

    int clampedAddressRange() const
    {
        const int start = clampedAddressStart();
        return std::clamp(addressRange, 1, kRamCellCount - start);
    }
};

inline std::vector<ScenarioDefinition> defaultScenarioDefinitions()
{
    ExperimentSettings baseline{};
    baseline.accessesPerSecond = 12.0f;

    ScenarioDefinition sequential;
    sequential.name = "sequential_access";
    sequential.description = "Contiguous RAM traversal. Shows spatial locality inside cache lines.";
    sequential.pattern = ScenarioPatternType::Sequential;
    sequential.steps = 128;
    sequential.settings = baseline;

    ScenarioDefinition random;
    random.name = "random_access";
    random.description = "Seeded random addresses across RAM. Produces weaker locality.";
    random.pattern = ScenarioPatternType::Random;
    random.seed = 7;
    random.steps = 128;
    random.settings = baseline;

    ScenarioDefinition stride;
    stride.name = "stride_access";
    stride.description = "Fixed stride traversal. Useful to show when strides skip cache-line reuse.";
    stride.pattern = ScenarioPatternType::Stride;
    stride.steps = 128;
    stride.stride = kDefaultCacheLineSize;
    stride.settings = baseline;

    ScenarioDefinition thrash;
    thrash.name = "cache_thrashing";
    thrash.description = "Cycles through one more line than the cache can hold, forcing repeated evictions.";
    thrash.pattern = ScenarioPatternType::CacheThrash;
    thrash.steps = 128;
    thrash.settings = baseline;

    ScenarioDefinition hotSet;
    hotSet.name = "repeated_hot_set";
    hotSet.description = "Repeatedly touches a small working set that fits in cache.";
    hotSet.pattern = ScenarioPatternType::RepeatedHotSet;
    hotSet.steps = 128;
    hotSet.hotSetSize = kDefaultCacheLineSize * 2;
    hotSet.settings = baseline;

    ScenarioDefinition soa;
    soa.name = "soa_scan";
    soa.description = "Simplified struct-of-arrays scan. Reads one dense field array.";
    soa.pattern = ScenarioPatternType::StructOfArrays;
    soa.steps = 128;
    soa.settings = baseline;

    ScenarioDefinition aos;
    aos.name = "aos_scan";
    aos.description = "Simplified array-of-structs scan. Jumps by field stride and wastes cache-line payload.";
    aos.pattern = ScenarioPatternType::ArrayOfStructs;
    aos.steps = 128;
    aos.stride = 4;
    aos.settings = baseline;

    return {sequential, random, stride, thrash, hotSet, soa, aos};
}
}

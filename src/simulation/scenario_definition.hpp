
#pragma once

#include <string>

namespace memory_playground
{
enum class AccessPatternType
{
    Sequential,
    Random,
    LinkedList,
    Strided
};

struct ScenarioDefinition
{
    std::string name;
    std::string description;

    unsigned int seed = 7;
    int steps = 256;

    AccessPatternType patternType = AccessPatternType::Sequential;

    int addressRangeMin = 0;
    int addressRangeMax = 127;

    int stride = 1;
    float readWriteRatio = 1.0f;
};
}

#pragma once

#include "simulation/constants.hpp"
#include "simulation/memory.hpp"

#include <random>
#include <vector>

namespace memory_playground
{
class AccessPattern
{
public:
    virtual ~AccessPattern() = default;
    virtual void reset(const std::vector<MemoryCell>& memory) = 0;
    virtual int nextAddress(const std::vector<MemoryCell>& memory) = 0;
    virtual const char* name() const = 0;
    virtual const char* description() const = 0;
};

class SequentialPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>&) override
    {
        cursor = 0;
    }

    int nextAddress(const std::vector<MemoryCell>& memory) override
    {
        const int address = cursor;
        cursor = (cursor + 1) % static_cast<int>(memory.size());
        return address;
    }

    const char* name() const override
    {
        return "Sequential Array Traversal";
    }

    const char* description() const override
    {
        return "Contiguous addresses reuse the cache line that was just loaded.";
    }

private:
    int cursor = 0;
};

class RandomPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>& memory) override
    {
        distribution = std::uniform_int_distribution<int>(0, static_cast<int>(memory.size()) - 1);
        generator.seed(7);
    }

    int nextAddress(const std::vector<MemoryCell>&) override
    {
        return distribution(generator);
    }

    const char* name() const override
    {
        return "Random Access Traversal";
    }

    const char* description() const override
    {
        return "Random jumps often land in cache lines that are not loaded.";
    }

private:
    std::mt19937 generator{7};
    std::uniform_int_distribution<int> distribution{0, kRamCellCount - 1};
};

class LinkedListPattern final : public AccessPattern
{
public:
    void reset(const std::vector<MemoryCell>& memory) override
    {
        current = memory.empty() ? 0 : memory.front().address;
    }

    int nextAddress(const std::vector<MemoryCell>& memory) override
    {
        const int address = current;
        current = memory[address].nextAddress;
        return address;
    }

    const char* name() const override
    {
        return "Linked List Traversal";
    }

    const char* description() const override
    {
        return "Each node points to the next address, so traversal jumps around RAM.";
    }

private:
    int current = 0;
};

enum class PatternKind
{
    Sequential,
    Random,
    LinkedList
};
}

#pragma once

#include "simulation/constants.hpp"

#include <algorithm>
#include <array>

namespace memory_playground
{
struct RegisterSlot
{
    bool valid = false;
    int address = -1;
    int value = 0;
    float flash = 0.0f;
};

class RegisterFile
{
public:
    int write(int address, int value)
    {
        const int slot = nextSlot;
        slots[slot] = RegisterSlot{true, address, value, 0.6f};
        nextSlot = (nextSlot + 1) % static_cast<int>(slots.size());
        return slot;
    }

    void reset()
    {
        for (auto& slot : slots)
        {
            slot = RegisterSlot{};
        }
        nextSlot = 0;
    }

    void update(float dt)
    {
        for (auto& slot : slots)
        {
            slot.flash = std::max(0.0f, slot.flash - dt);
        }
    }

    const std::array<RegisterSlot, kRegisterCount>& getSlots() const
    {
        return slots;
    }

private:
    std::array<RegisterSlot, kRegisterCount> slots{};
    int nextSlot = 0;
};
}

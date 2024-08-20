//
// Created by Boris on 20/08/2024.
//

#include "Counter.hpp"

namespace CppRtos
{
    Counter::Counter(std::uint32_t maxValue, std::uint32_t minCyc)
    : maxAllowedValue(maxValue)
    , minCycle(minCyc)
    , currentTick(0u)
    {
    }

    // Increment the counter
    void Counter::Increment()
    {
        currentTick++;
        if (currentTick >= maxAllowedValue)
        {
            currentTick = 0u; // Handle overflow
        }
    }

    // Calculate elapsed time considering overflow
    std::uint32_t Counter::GetElapsedValue(std::uint32_t startTick) const
    {
        if (currentTick >= startTick)
        {
            return currentTick - startTick;
        }
        else
        {
            return (maxAllowedValue - startTick) + currentTick;
        }
    }

    // Set counter value
    void Counter::SetCounterValue(std::uint32_t value)
    {
        if (value <= maxAllowedValue)
        {
            currentTick = value;
        }
    }
} // CppRtos
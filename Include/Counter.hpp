//
// Created by Boris on 20/08/2024.
//

#ifndef COUNTER_H
#define COUNTER_H


#include <cstdint>

namespace CppRtos
{

    class Counter
    {
    public:
        // Constructor
        Counter(std::uint32_t maxValue,std::uint32_t minCyc);

        // Increment the counter
        void Increment();

        // Get the current value of the counter
        [[nodiscard]] inline std::uint32_t GetCurrentTick() const
        {
            return currentTick;
        }

        // Calculate elapsed time considering overflow
        [[nodiscard]] std::uint32_t GetElapsedValue(std::uint32_t startTick) const;

        // Set counter value
        void SetCounterValue(std::uint32_t value);

    private:
        std::uint32_t maxAllowedValue   = 0u;
        std::uint32_t minCycle          = 0u;
        std::uint32_t currentTick       = 0u;
    };
} // CppRtos

#endif //COUNTER_H

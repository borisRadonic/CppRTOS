//
// Created by Boris on 03/08/2024.
//

#ifndef EVENTGROUP_H
#define EVENTGROUP_H

#include <cstdint>
#include "Config.hpp"
#include "Kernel.hpp"
#include "Fifo.hpp"

namespace CppRtos
{
    class EventGroup
    {
    public:

        EventGroup();

        // Wait for a specific set of flags to be set
        bool waitForEvents(std::uint32_t flagsToWaitFor, std::uint32_t timeout = WAIT_FOREVER);

        // Set a specific set of flags
        void setEvents(std::uint32_t flagsToSet);

        // Clear a specific set of flags
        void clearEvents(std::uint32_t flagsToClear);

        // Get the current event flags
        std::uint32_t getEvents() const;

    private:
        uint32_t eventFlags; // Current event flags
        Fifo<TaskData*, Settings::MAX_TASKS> waitingTasks; // Tasks waiting on events

        Kernel* ptrKernel = nullptr; // Pointer to the kernel
    };
}


#endif //EVENTGROUP_H

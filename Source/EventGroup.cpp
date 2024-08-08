//
// Created by Boris on 03/08/2024.
//

#include "EventGroup.h"
#include "KernelFactory.hpp"
#include "Kernel.hpp"

namespace CppRtos
{
    EventGroup::EventGroup() : eventFlags(0u)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        this->ptrKernel = ptrKernelFactory.getKernel();
    }

    bool EventGroup::waitForEvents(std::uint32_t flagsToWaitFor, std::uint32_t timeout)
    {
        ptrKernel->enterCritical();

        // Check if the required flags are already set
        if ((eventFlags & flagsToWaitFor) != 0)
        {
            ptrKernel->exitCritical();
            return true;
        }

        // If timeout is zero, return immediately as no wait is allowed
        if (timeout == 0)
        {
            ptrKernel->exitCritical();
            return false;
        }

        // Calculate the end time based on the current tick count and timeout
        std::uint64_t endTime = ptrKernel->getTickCount() + timeout;

        // Get the current task and block it, storing the flags it's waiting for
        TaskData* task = ptrKernel->getCurrentTask();
        task->setState(TaskStateType::eBlocked);
        task->setFlagsToWaitFor(flagsToWaitFor);  // Store the flags the task is waiting for
        waitingTasks.enqueue(task);

        ptrKernel->exitCritical();
        ptrKernel->yield();

        // Loop to periodically check if any event flags are set or if the timeout has expired
        while (true)
        {
            ptrKernel->enterCritical();

            // Check if any of the event flags are set
            if ((eventFlags & flagsToWaitFor) != 0)
            {
                // Remove the task from the waiting queue since the condition is met
                for (std::size_t i = 0; i < waitingTasks.getSize(); ++i)
                {
                    if (waitingTasks.getAt(i) == task)
                    {
                        waitingTasks.removeAt(i);
                        break;
                    }
                }
                ptrKernel->exitCritical();
                return true;
            }

            // Check if the current time exceeds the end time, meaning timeout has occurred
            if (ptrKernel->getTickCount() >= endTime)
            {
                // Remove the task from the waiting queue since it has timed out
                for (std::size_t i = 0; i < waitingTasks.getSize(); ++i)
                {
                    if (waitingTasks.getAt(i) == task)
                    {
                        waitingTasks.removeAt(i);
                        break;
                    }
                }
                ptrKernel->exitCritical();
                return false;
            }

            ptrKernel->exitCritical();
            ptrKernel->yield();
        }
    }

    void EventGroup::setEvents(std::uint32_t flagsToSet)
    {
        ptrKernel->enterCritical();

        eventFlags |= flagsToSet;

        // Process each task in the waiting queue
        for (std::size_t i = 0; i < waitingTasks.getSize();)
        {
            TaskData* task = waitingTasks.getAt(i);
            if ((eventFlags & task->getFlagsToWaitFor()) != 0u)
            {
                ptrKernel->setTaskReady(task);
                task->setFlagsToWaitFor(0u);
                waitingTasks.removeAt(i);  // Remove the task after setting it as ready
            }
            else
            {
                ++i;  // Only increment if no removal to avoid skipping elements
            }
        }

        ptrKernel->exitCritical();
        ptrKernel->yield();
    }

    void EventGroup::clearEvents(std::uint32_t flagsToClear)
    {
        ptrKernel->enterCritical();
        eventFlags &= ~flagsToClear;
        ptrKernel->exitCritical();
    }

    std::uint32_t EventGroup::getEvents() const
    {
        return eventFlags;
    }
}
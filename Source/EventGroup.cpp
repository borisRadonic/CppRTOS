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
        if ((eventFlags & flagsToWaitFor) != 0u)
        {
            ptrKernel->exitCritical();
            return true;
        }

        // If timeout is zero (or called from an ISR), return immediately as no wait is allowed
        if ((timeout == 0u) || ptrKernel->isInsideInterrupt())
        {
            ptrKernel->exitCritical();
            return false;
        }

        // Block the current task, storing the flags it's waiting for
        TaskData* task = ptrKernel->getCurrentTask();
        task->setFlagsToWaitFor(flagsToWaitFor);
        ptrKernel->blockCurrentTask(waitingTasks, timeout);
        ptrKernel->exitCritical();
        ptrKernel->yield();

        // Resumed: woken by setEvents() or by the timeout
        ptrKernel->enterCritical();
        const WakeReason reason = task->getWakeReason();
        task->setFlagsToWaitFor(0u);
        ptrKernel->exitCritical();

        return (reason == WakeReason::eSignaled);
    }

    void EventGroup::setEvents(std::uint32_t flagsToSet)
    {
        bool switchRequired = false;

        ptrKernel->enterCritical();
        eventFlags |= flagsToSet;

        // Wake every waiting task whose flags are now set
        std::uint32_t mask = waitingTasks.getMask();
        for (std::size_t id = 0u; (id < Settings::MAX_TASKS) && (mask != 0u); ++id)
        {
            if ((mask & (1u << id)) != 0u)
            {
                mask &= ~(1u << id);
                TaskData* task = ptrKernel->getTask(id);
                if ((task != nullptr) && ((eventFlags & task->getFlagsToWaitFor()) != 0u))
                {
                    ptrKernel->wakeTask(task, WakeReason::eSignaled, switchRequired);
                }
            }
        }
        ptrKernel->exitCritical();

        if (switchRequired)
        {
            ptrKernel->yield();
        }
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

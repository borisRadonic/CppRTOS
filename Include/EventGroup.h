//
// Created by Boris on 03/08/2024.
//

#ifndef EVENTGROUP_H
#define EVENTGROUP_H

#include <cstdint>
#include "Config.hpp"
#include "Fifo.hpp"
#include "Task.hpp"

namespace CppRtos
{
    class Kernel;

    /**
     * @class EventGroup
     * @brief Manages a set of event flags for task synchronization in an RTOS.
     *
     * The EventGroup class provides mechanisms for tasks to wait for and signal specific event flags.
     * Tasks can wait for one or more event flags to be set, and other tasks or ISRs can signal
     * these events, which will unblock the waiting tasks.
     *
     * The EventGroup class supports the following operations:
     * - Waiting for a specific set of event flags with an optional timeout.
     * - Setting event flags to unblock waiting tasks.
     * - Clearing specific event flags.
     * - Querying the current state of event flags.
     */
    class EventGroup
    {
    public:

        /**
        * @brief Constructs an EventGroup with all flags cleared.
        *
        * Initializes the EventGroup, setting all event flags to zero and preparing the
        * internal structures for managing tasks waiting on these flags.
        */
        EventGroup();

        /**
        * @brief Deleted copy constructor to prevent copying of EventGroup objects.
        */
        EventGroup(const EventGroup&) = delete;

        /**
        * @brief Deleted copy assignment operator to prevent copying of EventGroup objects.
        */
        EventGroup& operator=(const EventGroup&) = delete;

        /**
         * @brief Deleted move constructor to prevent moving of EventGroup objects.
         */
        EventGroup(EventGroup&&) = delete;

        /**
         * @brief Deleted move assignment operator to prevent moving of EventGroup objects.
         */
        EventGroup& operator=(EventGroup&&) = delete;

        /**
        * @brief Waits for a specific set of flags to be set.
        *
        * This method blocks the calling task until the specified event flags are set or
        * the optional timeout period expires. If the flags are already set, the function
        * returns immediately. If the timeout is zero, the function returns immediately
        * without waiting.
        *
        * @param flagsToWaitFor The set of flags to wait for.
        * @param timeout The maximum time (in system ticks) to wait for the flags. Defaults to WAIT_FOREVER.
        * @return True if the flags were set within the timeout period, false if the timeout occurred.
        */
        bool waitForEvents(std::uint32_t flagsToWaitFor, std::uint32_t timeout);

       /**
       * @brief Sets a specific set of flags.
       *
       * Signals the specified event flags, potentially unblocking tasks that are waiting
       * for these flags. If any tasks are waiting on the flags that are set, they will be
       * unblocked and scheduled to run.
       *
       * @param flagsToSet The set of flags to set.
       */
        void setEvents(std::uint32_t flagsToSet);

        /**
        * @brief Clears a specific set of flags.
        *
        * Clears the specified event flags, resetting them to zero. This operation does not
        * affect tasks that are currently waiting for these flags, but it prevents future
        * tasks from being unblocked by these flags until they are set again.
        *
        * @param flagsToClear The set of flags to clear.
        */
        void clearEvents(std::uint32_t flagsToClear);

        /**
        * @brief Gets the current state of the event flags.
        *
        * Returns the current state of all event flags managed by this EventGroup.
        *
        * @return The current event flags as a 32-bit unsigned integer.
        */
        [[nodiscard]] std::uint32_t getEvents() const;

    private:

        /**
        * @brief The current event flags.
        *
        * A 32-bit integer representing the state of the event flags. Each bit in this integer
        * corresponds to a specific event flag, where a bit value of 1 indicates that the flag
        * is set, and a bit value of 0 indicates that the flag is cleared.
        */
        uint32_t eventFlags;

        /**
        * @brief Queue of tasks waiting on events.
        *
        * A FIFO queue that stores pointers to tasks that are currently blocked, waiting for
        * specific event flags to be set. When the flags are set, the tasks are unblocked and
        * removed from this queue.
        */
        Fifo<TaskData*, Settings::MAX_TASKS> waitingTasks; // Tasks waiting on events

        /**
        * @brief Pointer to the kernel instance.
        *
        * A pointer to the Kernel instance that manages the overall scheduling and task management
        * in the RTOS. This allows the EventGroup to interact with the kernel, such as blocking and
        * unblocking tasks.
        */
        Kernel* ptrKernel; // Pointer to the kernel
    };
}


#endif //EVENTGROUP_H

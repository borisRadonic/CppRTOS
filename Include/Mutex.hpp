#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "Config.hpp"
#include "Kernel.hpp"
#include "Fifo.hpp"


namespace CppRtos
{
    /**
     * @brief Enumeration representing the result of mutex operations.
     */
    enum class MutexResult : std::uint8_t
    {
        Success             = 0u, /**< Operation succeeded */
        ErrorCalledFromISR  = 1u, /**< Operation failed because it was called from an ISR */
        Timeout             = 2u, /**< Operation timed out */
        AlreadyOwned        = 3u, /**< Operation failed because the mutex is already owned by the task */
        NotOwner            = 4u  /**< Operation failed because the mutex is being released by a task that does not own it */
    };

    /**
     * @brief Mutex class for synchronizing tasks in a Real-Time Operating System (RTOS).
     * 
     * The Mutex class provides a mechanism for tasks to serialize access to a shared resource.
     * A mutex ensures that only one task can own the mutex at any given time.
     * 
     * Key features:
     * - Tasks can acquire the mutex and will block if the mutex is not available.
     * - The mutex can only be acquired by tasks, not by Interrupt Service Routines (ISRs).
     * - A `release` method is provided to release the mutex, allowing other tasks to acquire it.
     */
    class Mutex
    {
    public:

        /**
         * @brief Constructs a Mutex.
         */
        Mutex();

        /**
         * @brief Destructs the Mutex.
         * 
         * Ensures no tasks are blocked when the mutex is destroyed.
         */
        ~Mutex();
        
        /**
         * @brief Attempts to acquire the mutex, potentially blocking the task.
         * 
         * If the mutex is not available, the task will be blocked until the mutex
         * is released by another task or an optional timeout occurs.
         * 
         * @param ticksTimeout The number of ticks to wait before timing out. Use WAIT_FOREVER to wait indefinitely.
         * @return MutexResult The result of the acquire operation.
         * - MutexResult::Success: The mutex was successfully acquired.
         * - MutexResult::ErrorCalledFromISR: The acquire operation was called from an ISR, which is not allowed.
         * - MutexResult::Timeout: The acquire operation timed out before the mutex became available.
         * - MutexResult::AlreadyOwned: The mutex is already owned by the task attempting to acquire it.
         */
        MutexResult acquire(std::uint32_t ticksTimeout);
       
        /**
         * @brief Releases the mutex, allowing other tasks to acquire it.
         * 
         * If there are tasks waiting on the mutex, one of them will be unblocked
         * and allowed to acquire the mutex. If no tasks are waiting, the mutex
         * is released and becomes available.
         * 
         * @return MutexResult The result of the release operation.
         * - MutexResult::Success: The mutex was successfully released.
         * - MutexResult::ErrorCalledFromISR: The release operation was called from an ISR, which is allowed.
         * - MutexResult::NotOwner: The release operation was attempted by a task that does not own the mutex.
         */
        MutexResult release();

    private:

        /**
         * @brief Checks if any tasks in the blocked queue have the given priority.
         * 
         * @param priority The priority level to check for blocking tasks.
         * @return true if there is any task blocking at the given priority level, false otherwise.
         */
        bool isAnyTaskBlocking(CppRtos::TaskPriority priority);
       
    private:
        std::int32_t count = 0; /**< The count of the mutex */
        TaskData* owner = nullptr; /**< Pointer to the task that currently owns the mutex */
        Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks = {}; /**< Queue of tasks waiting on the mutex */
    };
}

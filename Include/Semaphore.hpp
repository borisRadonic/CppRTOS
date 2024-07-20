#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include "Config.hpp"
#include "Fifo.hpp"
#include "Kernel.hpp"

namespace CppRtos
{
     /**
     * @brief Enumeration representing the result of semaphore operations.
     */
    enum class SemResult : std::uint8_t
    {
        Success             = 0u,           /**< Operation succeeded */
        ErrorCalledFromISR  = 1u,           /**< Operation failed because it was called from an ISR */
        Timeout  = 2u                       /**< Operation timed out */
    };

    /**
     * @brief Semaphore class for synchronizing tasks in a Real-Time Operating System (RTOS).
     * 
     * The Semaphore class provides a mechanism for tasks to synchronize their operations
     * or to serialize access to a shared resource. A semaphore maintains a count that is
     * decremented when a task acquires the semaphore and incremented when a task releases it.
     * 
     * Key features:
     * - Tasks can acquire the semaphore and will block if the semaphore is not available.
     * - The semaphore can be released from both tasks and Interrupt Service Routines (ISRs).
     * - A `flush` method is provided to unblock all tasks waiting on the semaphore.
     * 
     * Note: The semaphore can be given (released) from an ISR but not taken (acquired) from an ISR.
     */
    class Semaphore
    {
    public:
        
        /**
         * @brief Constructs a Semaphore with a maximum count and an initial count.
         * 
         * @param maxCount The maximum count the semaphore can reach.
         * @param initialCount The initial count of the semaphore.
         */
        explicit Semaphore(std::uint32_t maxCount, std::uint32_t initialCount );
       
      /**
         * @brief Attempts to acquire the semaphore, potentially blocking the task.
         * 
         * If the semaphore is not available, the task will be blocked until the semaphore
         * is released by another task or an optional timeout occurs.
         * 
         * @param ticksTimeout The number of ticks to wait before timing out. Use WAIT_FOREVER to wait indefinitely.
         * @return SemResult The result of the acquire operation.
         * - SemResult::Success: The semaphore was successfully acquired.
         * - SemResult::ErrorCalledFromISR: The acquire operation was called from an ISR, which is not allowed.
         * - SemResult::Timeout: The acquire operation timed out before the semaphore became available.
         */
        SemResult acquire(std::uint32_t ticksTimeout);
       
      /**
         * @brief Releases the semaphore, potentially unblocking a waiting task.
         * 
         * If there are tasks waiting on the semaphore, one of them will be unblocked
         * and allowed to acquire the semaphore. If no tasks are waiting, the semaphore
         * count is incremented.
         */
        void release();

       /**
         * @brief Atomically unblocks all tasks pending on the semaphore queue.
         * 
         * This method will make all tasks ready before any task actually executes.
         * The count of the semaphore will remain unchanged.
         * 
         * This can be useful in scenarios where a reset or re-initialization is needed,
         * and all waiting tasks need to be unblocked.
         */
        void flush();
       
    private:

       std::uint32_t maxCount; /**< The maximum count the semaphore can reach */

        std::uint32_t count; /**< The current count of the semaphore */
        
        Fifo<TaskData*, Settings::MAX_TASKS> waitingQueue  = {}; /**< Queue of tasks waiting on the semaphore */

        Kernel* ptrKernel = nullptr; /**< Pointer to the singleton kernel instance */

    };
}

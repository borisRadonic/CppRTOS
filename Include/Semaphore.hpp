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
     * @class Semaphore
     * @brief A synchronization primitive for task management in a Real-Time Operating System (RTOS).
     *
     * The Semaphore class enables tasks to synchronize their operations or control access to shared resources.
     * It manages a count that tracks the number of available "tokens" or permits. Tasks can acquire or release
     * these tokens to coordinate their execution, ensuring that only a specified number of tasks can access a
     * resource or proceed at any given time.
     *
     * Key features:
     * - **Blocking wait**: Tasks can attempt to acquire the semaphore, and will block (i.e., be suspended) if
     *   the semaphore is unavailable. The task remains blocked until the semaphore is released by another task
     *   or an ISR, and the semaphore count becomes positive.
     * - **ISR compatibility**: The semaphore can be released (signaled) from both tasks and Interrupt Service
     *   Routines (ISRs), allowing for flexible synchronization even in interrupt contexts. However, semaphores
     *   should not be acquired (waited on) from an ISR due to the potential for deadlock or unpredictable behavior.
     * - **Flush operation**: A `flush()` method is provided to unblock all tasks currently waiting on the semaphore,
     *   regardless of the semaphore's count. This can be useful for situations where tasks need to be released
     *   simultaneously, such as during shutdown procedures or when a critical event occurs.
     *
     * Usage notes:
     * - Semaphores are ideal for managing limited resources, such as controlling access to a pool of buffers,
     *   serializing access to a shared resource, or coordinating tasks in complex workflows.
     * - Be cautious when using semaphores in ISRs: only the release operation is safe, while acquisition should
     *   be restricted to tasks to avoid complications.
     *
     * Example:
     * @code
     * Semaphore semaphore(1);  // Binary semaphore with an initial count of 1
     *
     * void Task1() {
     *     if (semaphore.acquire()) {
     *         // Access the shared resource
     *         semaphore.release();
     *     }
     * }
     *
     * void ISR_Handler() {
     *     semaphore.release();  // Safely release the semaphore from the ISR
     * }
     * @endcode
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
        SemResult wait(std::uint32_t ticksTimeout);
       
      /**
         * @brief Releases (signals) the semaphore, potentially unblocking a waiting task.
         * 
         * If there are tasks waiting on the semaphore, one of them will be unblocked
         * and allowed to acquire the semaphore. If no tasks are waiting, the semaphore
         * count is incremented.
         */
        void signal();

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

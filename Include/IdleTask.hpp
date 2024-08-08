#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "Task.hpp"

namespace CppRtos
{
    constexpr std::size_t IDLE_TASK_STACK_SIZE = 1024u;

    /**
 * @brief Class representing the idle task in the Kernel.
 *
 * The IdleTask class implements the task that runs when no other tasks are ready to execute.
 * It inherits from the Task class and provides a simple, infinite loop that keeps the CPU busy
 * when there is nothing else to do. The idle task typically has the lowest priority in the system.
 */
    class IdleTask : public Task
    {
    public:
        /**
         * @brief Constructs an IdleTask with a predefined stack size.
         *
         * Initializes the idle task by setting up its stack memory and configuring it with the lowest priority.
         * The stack memory is statically allocated and aligned to meet the required alignment constraints.
         */
        IdleTask() : Task(static_cast<std::uint8_t*>(static_cast<void*>(&stack)), IDLE_TASK_STACK_SIZE)
        {
        }

        /**
         * @brief Destructor for the IdleTask class.
         *
         * The destructor is defaulted, as the idle task does not require any special cleanup.
         */
        ~IdleTask() override = default;

        /**
         * @brief The main loop of the idle task.
         *
         * This function implements the idle task's main functionality, which is an infinite loop.
         * The idle task runs continuously when no other tasks are ready to execute.
         */
        void run() override
        {
            while (true)
            {
                // Idle task does nothing but loop indefinitely.
            }
        }

    public:
        /**
         * @brief Stack memory for the idle task.
         *
         * The stack is statically allocated using aligned storage to ensure it meets the alignment requirements.
         * This stack is used by the idle task to maintain its execution context.
         */
        std::aligned_storage_t<IDLE_TASK_STACK_SIZE, 4u> stack{};
    };
}

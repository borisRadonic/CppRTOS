#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

/**
 * @brief Interface class for port-specific operations in a real-time operating system (RTOS).
 *
 * This class provides a set of virtual functions that must be implemented by any port-specific
 * layer in an RTOS. It handles tasks such as interrupt management, critical section management,
 * and stack initialization, which are crucial for the RTOS's operation on different hardware platforms.
 */
class IPort
{
public:
    /**
     * @brief Default constructor for the IPort interface.
     *
     * This constructor does not perform any initialization. It is expected that derived classes
     * will handle any necessary setup.
     */
    IPort() = default;

    /**
     * @brief Virtual destructor for the IPort interface.
     *
     * Ensures that derived classes can clean up resources properly when an IPort object is destroyed.
     */
    virtual ~IPort() = default;

    /**
     * @brief Checks if the current execution context is within an interrupt.
     *
     * @return true if the current execution context is inside an interrupt, false otherwise.
     */
    [[nodiscard]] virtual bool isInsideInterrupt() const = 0;

    /**
     * @brief Disables interrupts on the current processor.
     *
     * Prevents all interrupts from occurring. This function is typically used in critical sections
     * to ensure atomic operations.
     */
    virtual void disableInterrupts() const = 0;

    /**
     * @brief Enables interrupts on the current processor.
     *
     * Re-enables interrupts that were previously disabled. This function should be called
     * after a critical section or when interrupt-safe operations are complete.
     */
    virtual void enableInterrupts() const = 0;

    /**
     * @brief Enters a critical section.
     *
     * A critical section is a portion of code that must not be interrupted to prevent data corruption
     * or other errors. This function should disable interrupts or otherwise prevent context switches.
     */
    virtual void enterCritical() = 0;

    /**
     * @brief Exits a critical section.
     *
     * Reverses the effects of enterCritical(), re-enabling interrupts or allowing context switches to occur.
     */
    virtual void exitCritical() = 0;

    /**
     * @brief Ensures memory operations are completed in the intended order.
     *
     * A memory barrier prevents reordering of memory operations, which is crucial in certain
     * low-level programming contexts, particularly in multi-core systems.
     */
    virtual void memoryBarrier() const = 0;

    /**
     * @brief Validates that the current interrupt priority is safe for critical operations.
     *
     * This function is used to check if the current interrupt priority is within a range
     * that allows critical operations to be performed safely.
     */
    virtual void validateInterruptPriority() const = 0;

    /**
     * @brief Starts the RTOS scheduler.
     *
     * This function starts the scheduler, which begins managing tasks and their execution.
     * It typically does not return unless the scheduler is explicitly stopped.
     */
    virtual void startScheduler() = 0;

    /**
     * @brief Ends the RTOS scheduler.
     *
     * Stops the scheduler, effectively halting task management and allowing the system
     * to cease multitasking.
     */
    virtual void endScheduler() const = 0;

    /**
     * @brief Handles a task exit error.
     *
     * This function is called when a task exits in an unexpected or erroneous manner. It typically
     * handles cleanup and ensures the RTOS remains stable.
     */
    virtual void taskExitError() = 0;

    /**
     * @brief Initializes a task stack for use by the RTOS.
     *
     * Sets up the stack for a new task, ensuring that the task's initial context is correctly configured.
     *
     * @param pxTopOfStack Pointer to the top of the stack where the task's stack frame should be created.
     * @param pvParameters Pointer to the parameters that will be passed to the task function.
     * @return Pointer to the initialized top of the stack.
     */
    virtual void* initialiseStack(void* pxTopOfStack, void* pvParameters) = 0;
};


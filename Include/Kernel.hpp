#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "Port.hpp"
#include "Config.hpp"
#include "Task.hpp"
#include "Timer.hpp"
#include "EventGroup.h"
#include "IdleTask.hpp"
#include "Fifo.hpp"

namespace CppRtos
{
	constexpr std::uint32_t WAIT_FOREVER = 0xFFFFFFFF;

	/**
	* @brief Represents the state of the Kernel.
	*
	* This enum defines various states that the Kernel can be in during its lifecycle.
	*/
	enum class KernelState : std::uint8_t
	{
		eReset = 0u,   /**< The Kernel is in the reset state. */
		eReady = 1u,   /**< The Kernel is ready but not yet running. */
		eRunning = 2u, /**< The Kernel is currently running. */
		eError = 0xFFu /**< The Kernel has encountered an error. */
	};

	/**
	 * @brief Represents various error codes for Kernel operations.
	 *
	 * This enum defines error codes that indicate the status of Kernel operations.
	 */
	enum class KernelError : std::uint32_t
	{
		eOK           = 0u, /**< Operation completed successfully. */
		eNotReady     = 1u, /**< Operation attempted when the Kernel is not ready. */
		eInInterrupt  = 2u, /**< Operation attempted during an interrupt. */
		eNotRunning   = 3u, /**< Operation attempted when the Kernel is not running. */
		eLocked       = 4u  /**< Operation failed due to a locked state. */
	};

	/**
	* @brief The Kernel class manages the real-time operating system's core functionality.
	*
	* The Kernel class is responsible for managing tasks, timers, interrupts, and the system's
	* overall state. It provides mechanisms to start the scheduler, manage task priorities,
	* and ensure safe operations in a multi-tasking environment.
	*/
    class Kernel final
    {
	public:

    	/**
	 * @brief Deleted copy constructor to prevent copying of the Kernel instance.
	 */
    	Kernel(const Kernel&) = delete;

    	/**
		 * @brief Deleted copy assignment operator to prevent copying of the Kernel instance.
		 */
    	Kernel& operator=(const Kernel&) = delete;

    	/**
		 * @brief Deleted move constructor to prevent moving of the Kernel instance.
		 */
    	Kernel(Kernel&&) = delete;

    	/**
		 * @brief Deleted move assignment operator to prevent moving of the Kernel instance.
		 */
    	Kernel& operator=(Kernel&&) = delete;

    	/**
		 * @brief Adds a task to the Kernel.
		 *
		 * This function registers a new task with the Kernel, enabling it to be scheduled.
		 *
		 * @param task Reference to the Task object to be added.
		 */
    	void addTask(Task& task);

    	/**
		 * @brief Adds a timer to the Kernel.
		 *
		 * Registers a timer with the Kernel for periodic execution.
		 *
		 * @param timer Pointer to the Timer object to be added.
		 * @return true if the timer was successfully added, false otherwise.
		 */
    	bool addTimer(Timer* timer);

    	/**
		 * @brief Removes a timer from the Kernel.
		 *
		 * This function removes a previously added timer from the Kernel.
		 *
		 * @param timer Pointer to the Timer object to be removed.
		 */
    	void removeTimer(Timer* timer);

    	/**
		 * @brief Retrieves the underlying port interface.
		 *
		 * Provides access to the Port instance used by the Kernel for low-level operations.
		 *
		 * @return Reference to the Port object.
		 */
    	[[nodiscard]] inline Port::Port& getPort()
		{
			return this->port;
		}

    	/**
		* @brief Initializes the Kernel.
		*
		* Prepares the Kernel for operation, setting its state to ready.
		*/
        void initialize()
		{
			state = KernelState::eReady;
		}

    	/**
		* @brief Starts the Kernel scheduler.
		*
		* Begins task scheduling and management. This function does not return until the scheduler is stopped.
		*/
		void start();

    	/**
		 * @brief Retrieves the current system tick count.
		 *
		 * Returns the number of ticks since the Kernel started.
		 *
		 * @return The system tick count.
		 */
		[[nodiscard]] inline std::uint64_t getTickCount() const
		{
			return port.getTickCount();
		}

		[[nodiscard]] inline std::uint64_t getSysTimerCount() const
		{
			return port.getSysTimerCount();
		}

    	friend class EventGroup;
    	friend class Mutex;
    	friend class KernelFactory;
    	friend class Task;
    	friend class Semaphore;
    	friend class Port::Port;

    private:

    	/**
		* @brief Default constructor.
		*
		* Initializes the Kernel instance. This constructor is private to enforce singleton usage.
		*/
    	Kernel();

    	/**
		* @brief Default destructor.
		*/
    	~Kernel() = default;

    	/**
		 * @brief Updates the timers managed by the Kernel.
		 *
		 * This function checks and updates all timers, managing their expiration and execution.
		 */
    	void updateTimers() const;

    	/**
		 * @brief Retrieves the currently running task.
		 *
		 * Provides access to the task that is currently being executed by the Kernel.
		 *
		 * @return Pointer to the currently running TaskData.
		 */
    	[[nodiscard]] inline TaskData* getCurrentTask() const
    	{
    		return currentTask;
    	}

    	/**
		* @brief Sets the current task to the specified task.
		*
		* Changes the task currently being executed by the Kernel.
		*
		* @param task Pointer to the TaskData object representing the task to be set as current.
		*/
    	void setCurrentTask(TaskData* task)
    	{
    		currentTask = task;
    	}

    	/**
		* @brief Selects the task with the highest priority for execution.
		*
		* This function selects the highest-priority task that is ready to run.
		*/
    	void selectHighestPriorityTask();

    	/**
		 * @brief Handles the system tick event.
		 *
		 * This function is called on each system tick, performing time-based operations such as
		 * updating timers and scheduling tasks.
		 */
    	void tick();

    	/**
		 * @brief Adds a task to the sleeping task list.
		 *
		 * Puts the specified task into a sleep state for a given number of ticks.
		 *
		 * @param task Pointer to the TaskData object to be put to sleep.
		 * @param ticks The number of ticks the task should sleep.
		 */
    	void addSleepingTask(TaskData* task, std::uint32_t ticks);

    	/**
		 * @brief Marks a task as ready for execution.
		 *
		 * Moves a task from a blocked or waiting state to the ready state.
		 *
		 * @param ptrTask Pointer to the TaskData object to be marked as ready.
		 */
    	void setTaskReady( CppRtos::TaskData * ptrTask );

    	/**
		 * @brief Resets a task's ready state.
		 *
		 * Moves a task from the ready state to a new state, typically to block it from execution.
		 *
		 * @param ptrTask Pointer to the TaskData object to be updated.
		 * @param newState The new state to set for the task.
		 */
    	void resetTaskReady(CppRtos::TaskData* ptrTask, TaskStateType newState);

    	/**
		* @brief Enters a critical section.
		*
		* Disables interrupts or otherwise prevents context switches, ensuring atomic operations.
		*/
    	inline void enterCritical()
    	{
    		port.enterCritical();
    	}

    	/**
		 * @brief Exits a critical section.
		 *
		 * Re-enables interrupts or allows context switches, ending atomic operations.
		 */
    	inline void exitCritical()
    	{
    		port.exitCritical();
    	}

    	/**
		 * @brief Retrieves a task by its index.
		 *
		 * Provides access to a task in the task list by its index.
		 *
		 * @param index The index of the task in the task list.
		 * @return Pointer to the TaskData object at the specified index, or nullptr if the index is out of bounds.
		 */
    	[[nodiscard]] TaskData* getTask(std::size_t index) const
    	{
    		if (index < tasks.size())
    		{
    			return tasks[index];
    		}
    		return nullptr;
    	}

    	/**
		 * @brief Sets a task at the specified index in the task list.
		 *
		 * Updates the task stored at the given index in the task list.
		 *
		 * @param index The index at which to set the task.
		 * @param taskData Pointer to the TaskData object to store at the index.
		 */
    	void setTask(std::size_t index, TaskData* taskData)
    	{
    		if (index < tasks.size())
    		{
    			tasks[index] = taskData;
    		}
    	}

    	/**
		* @brief Disables all interrupts on the current processor.
		*
		* Prevents any interrupts from occurring, typically used during critical sections.
		*/
    	inline void disableInterrupts() const
    	{
    		port.disableInterrupts();
    	}

    	/**
		* @brief Enables all interrupts on the current processor.
		*
		* Re-enables interrupts that were previously disabled.
		*/
    	inline void enableInterrupts() const
    	{
    		port.enableInterrupts();
    	}

    	/**
		* @brief Yields the processor to allow another task to run.
		*
		* Forces a context switch, allowing another task to be executed.
		*/
    	inline void yield()
    	{
    		port.yield();
    	}

    	/**
		* @brief Checks if the current execution context is within an interrupt.
		*
		* Determines whether the current code is running within an interrupt service routine.
		*
		* @return true if inside an interrupt, false otherwise.
		*/
    	[[nodiscard]] inline bool isInsideInterrupt( ) const
    	{
    		return port.isInsideInterrupt();
    	}

    private:

    	KernelState state = KernelState::eReset;								/**< The current state of the Kernel. */

    	KernelError lastError = KernelError::eOK;								/**< The last error code encountered by the Kernel. */

    	std::array<TaskData*, Settings::MAX_TASKS> tasks{};						/**< Array of task pointers managed by the Kernel. */

    	std::array<std::uint32_t, MAX_PRIORY_LEVELS> readyTasks = {};			/**< Array of bitmaps for task priorities. */

    	std::uint32_t priorityBitmap = 0u;										/**< Bitmap tracking non-empty priority levels. */

    	TaskData* currentTask = nullptr;										/**< Pointer to the currently executing task. */

    	TaskPriority highestTaskPriority = TaskPriority::PRIORITY_IDLE;			/**< Highest priority of the tasks ready to run. */

    	std::uint8_t taskCount = 0u;											/**< The current number of tasks added to the Kernel. */

    	Port::Port port;														/**< The port interface used for low-level operations. */

    	IdleTask idleTask;														/**< The idle task that runs when no other task is ready. */

    	std::uint64_t sleepingTasksBitmap = 0u;									/**< Bitmap tracking sleeping tasks. */

    	std::array<std::uint64_t, Settings::MAX_TASKS> taskWakeUpTimes = {0u};	/**< Array of wake-up times for sleeping tasks. */

    	std::array<Timer*, Settings::MAX_TIMERS> timers = {nullptr};			/**< Array of timers managed by the Kernel. */
    };
}

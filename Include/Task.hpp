#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>
#include "Config.hpp"

namespace CppRtos
{

	class ITask;

	constexpr std::uint32_t MAX_TASK_NAME = 16u;

	/**
	* @brief Enumeration representing the possible states of a task in the Kernel.
	*
	* The TaskStateType enum defines the different states a task can be in during its lifecycle
	* within the Kernel. These states are used to manage task scheduling, execution, and control.
	*/
	enum class TaskStateType : std::uint8_t
	{
		eReady      = 0u, /**< Task is ready to run but not currently executing. */
		eRunning    = 1u, /**< Task is currently executing. */
		eBlocked    = 2u, /**< Task is blocked, waiting for an event or resource. */
		eSuspended  = 3u, /**< Task is suspended and not eligible for execution. */
		eSleeping   = 4u  /**< Task is sleeping for a specified period. */
	};

	/**
 * @brief Enumeration representing the priority levels of tasks in the Kernel.
 *
 * The TaskPriority enum defines various priority levels that tasks can be assigned in the Kernel.
 * These priorities help the scheduler determine the order in which tasks should be executed,
 * with lower numerical values indicating higher priorities.
 */
	enum class TaskPriority : std::uint8_t
	{
		PRIORITY_HIGHEST           = 0u,   /**< Highest priority level. */
		PRIORITY_VERY_HIGH         = 1u,   /**< Very high priority level. */
		PRIORITY_HIGH              = 2u,   /**< High priority level. */
		PRIORITY_MODERATELY_HIGH   = 3u,   /**< Moderately high priority level. */
		PRIORITY_MEDIUM_HIGH       = 4u,   /**< Medium-high priority level. */
		PRIORITY_SLIGHTLY_HIGH     = 5u,   /**< Slightly high priority level. */
		PRIORITY_ABOVE_AVERAGE      = 6u,   /**< Above average priority level. */
		PRIORITY_AVERAGE           = 7u,   /**< Average priority level. */
		PRIORITY_BELOW_AVERAGE     = 8u,   /**< Below average priority level. */
		PRIORITY_SLIGHTLY_LOW      = 9u,   /**< Slightly low priority level. */
		PRIORITY_MEDIUM_LOW        = 10u,  /**< Medium-low priority level. */
		PRIORITY_MODERATELY_LOW    = 11u,  /**< Moderately low priority level. */
		PRIORITY_LOW               = 12u,  /**< Low priority level. */
		PRIORITY_VERY_LOW          = 13u,  /**< Very low priority level. */
		PRIORITY_LOWER_LOW         = 14u,  /**< Lower low priority level. */
		PRIORITY_IDLE              = 15u,  /**< Lowest priority level (idle tasks). */
		PRIORITY_RESERVED          = 0xFF  /**< Reserved priority level. */
	};

	constexpr std::uint8_t MAX_PRIORY_LEVELS = static_cast<std::uint8_t>(TaskPriority::PRIORITY_IDLE) + 1u;

	inline bool operator < (TaskPriority lhs, TaskPriority rhs)
	{
		return static_cast<uint8_t>(lhs) > static_cast<uint8_t>(rhs);
	}

	inline bool operator > (TaskPriority lhs, TaskPriority rhs)
	{
    	return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs);
	}

	using StackAddr = void*;
       
	/**
	 * @brief Class representing the internal data and state of a task in the Kernel.
	 *
	 * The TaskData class encapsulates all the necessary information about a task, including its stack pointers,
	 * priority levels, state, and various task-specific attributes. It is used by the Kernel to manage and
	 * schedule tasks effectively.
	 */
	class TaskData
	{
	public:
	    /**
	     * @brief Constructs a TaskData object with default values.
	     *
	     * Initializes the task's stack pointers, priority, state, and other attributes to default values.
	     */
	    explicit TaskData()
	        : currentStackPointer(nullptr),
	          priority(TaskPriority::PRIORITY_IDLE),
	          basePriority(TaskPriority::PRIORITY_IDLE),
	          flagsToWaitFor(0u),
	          state(TaskStateType::eReady),
	          id(0u),
	          startStack(nullptr),
	          endStack(nullptr),
	          wakeUpTime(0u)
	    {
	        name.fill(0);
	    }

	    /**
	     * @brief Default destructor.
	     */
	    ~TaskData() = default;

	    /**
	     * @brief Resets the task's priority to its base priority.
	     */
	    inline void resetPriority()
	    {
	        priority = basePriority;
	    }

	    /**
	     * @brief Sets the flags that the task is waiting for.
	     *
	     * @param flags The flags to set.
	     */
	    inline void setFlagsToWaitFor(const std::uint32_t flags)
	    {
	        flagsToWaitFor = flags;
	    }

	    /**
	     * @brief Gets the flags that the task is waiting for.
	     *
	     * @return The flags that the task is waiting for.
	     */
	    [[nodiscard]] inline std::uint32_t getFlagsToWaitFor() const
	    {
	        return flagsToWaitFor;
	    }

	    /**
	     * @brief Sets the top of the task's stack.
	     *
	     * @param topStack The address representing the top of the task's stack.
	     */
	    inline void setTopStack(StackAddr topStack)
	    {
	        endStack = topStack;
	        currentStackPointer = topStack; // Set the current stack pointer to the top at start
	    }

	    /**
	     * @brief Gets the current stack pointer of the task.
	     *
	     * @return The current stack pointer address.
	     */
	    [[nodiscard]] inline StackAddr getCurrentStackPtr() const
	    {
	        return currentStackPointer;
	    }

	    /**
	     * @brief Sets the start address of the task's stack.
	     *
	     * @param startStck The start address of the stack.
	     */
	    inline void setStartStack(StackAddr startStck)
	    {
	        this->startStack = startStck;
	    }

	    /**
	     * @brief Sets the current stack pointer to a specified address.
	     *
	     * @param currentStck The new current stack pointer address.
	     */
	    inline void setCurrentStackPtr(StackAddr currentStck)
	    {
	        this->currentStackPointer = currentStck;
	    }

	    /**
	     * @brief Gets the start address of the task's stack.
	     *
	     * @return The start stack address.
	     */
	    [[nodiscard]] inline StackAddr getStartStackddress() const
	    {
	        return startStack;
	    }

	    /**
	     * @brief Sets the task's priority.
	     *
	     * @param prio The new priority to set.
	     */
	    inline void setPriority(TaskPriority prio)
	    {
	        if (prio < TaskPriority::PRIORITY_IDLE)
	        {
	            this->priority = TaskPriority::PRIORITY_IDLE;
	            this->basePriority = prio;
	        }
	        else
	        {
	            this->priority = prio;
	            this->basePriority = prio;
	        }
	    }

	    /**
	     * @brief Gets the current priority of the task.
	     *
	     * @return The task's current priority.
	     */
	    [[nodiscard]] inline TaskPriority getPriority() const
	    {
	        return priority;
	    }

	    /**
	     * @brief Sets the base priority of the task.
	     *
	     * @param basePrio The base priority to set.
	     */
	    inline void setBasePriority(const TaskPriority basePrio)
	    {
	        this->basePriority = basePrio;
	    }

	    /**
	     * @brief Gets the base priority of the task.
	     *
	     * @return The task's base priority.
	     */
	    [[nodiscard]] inline TaskPriority getBasePriority() const
	    {
	        return basePriority;
	    }

	    /**
	     * @brief Sets the task interface pointer.
	     *
	     * @param taskIntPtr Pointer to the ITask interface.
	     */
	    inline void setTaskInterfacePtr(ITask* taskIntPtr)
	    {
	        taskInterfacePtr = taskIntPtr;
	    }

	    /**
	     * @brief Gets the task interface pointer.
	     *
	     * @return Pointer to the ITask interface.
	     */
	    [[nodiscard]] inline ITask* getTaskInterfacePtr() const
	    {
	        return taskInterfacePtr;
	    }

	    /**
	     * @brief Gets the end address of the task's stack.
	     *
	     * @return The end stack address.
	     */
	    [[nodiscard]] inline StackAddr getEndStackddress() const
	    {
	        return endStack;
	    }

	    /**
	     * @brief Sets the task's state.
	     *
	     * @param state The new state to set.
	     */
	    inline void setState(const TaskStateType state)
	    {
	        this->state = state;
	    }

	    /**
	     * @brief Gets the current state of the task.
	     *
	     * @return The task's current state.
	     */
	    [[nodiscard]] inline TaskStateType getState() const
	    {
	        return state;
	    }

	    /**
	     * @brief Sets the task's name.
	     *
	     * Copies the provided name into the task's name field, ensuring it is null-terminated.
	     *
	     * @param name The name to set for the task.
	     */
	    inline void setName(const std::string_view& name)
	    {
	        this->name.fill(0);
	        size_t num_to_copy = std::min(name.size(), name.size() - 1);
	        std::copy_n(name.begin(), num_to_copy, this->name.begin());
	    }

	    /**
	     * @brief Sets the task's unique identifier.
	     *
	     * @param id The ID to set for the task.
	     */
	    inline void setId(std::uint32_t id)
	    {
	        this->id = id;
	    }

	    /**
	     * @brief Gets the task's unique identifier.
	     *
	     * @return The task's ID.
	     */
	    [[nodiscard]] inline std::uint32_t getId() const
	    {
	        return id;
	    }

	private:

	    StackAddr currentStackPointer			= nullptr;						/**< Pointer to the current stack position. */

		TaskPriority priority					= TaskPriority::PRIORITY_IDLE;  /**< Current priority of the task. */

		TaskPriority basePriority				= TaskPriority::PRIORITY_IDLE;  /**< Base priority of the task, u// Used for mutexes by priority inheritance logic. */

		std::uint32_t flagsToWaitFor			= 0U;							/**< Flags that the task is waiting for. */

		TaskStateType state						= TaskStateType::eReady;        /**< Current state of the task. */

		std::uint32_t id						= 0u;						    /**< Unique identifier for the task. */

		std::array<char, MAX_TASK_NAME> name	= {};							/**< Name of the task, stored as a fixed-size array. */

		StackAddr startStack					= nullptr;						/**< Start address of the task's stack. */

		StackAddr endStack						= nullptr;						/**< End address of the task's stack. */

		std::uint64_t wakeUpTime				= 0u;							/**< Time when the task should wake up if it is sleeping. */

		ITask* taskInterfacePtr					= nullptr;						/**< Pointer to the task interface. */
	};

	/**
	* @brief Interface representing a task in the Kernel.
	*
	* The ITask interface defines the contract for any task that can be scheduled and executed by the Kernel.
	* Implementing classes must provide an implementation for the `run` method, which contains the task's main functionality.
	*/
	class ITask
	{
	public:
		/**
		 * @brief Executes the task's main functionality.
		 *
		 * This pure virtual function must be overridden by any class that implements the ITask interface.
		 * The `run` method is called by the Kernel to perform the task's operations.
		 */
		virtual void run() = 0;

		/**
		 * @brief Virtual destructor for the ITask interface.
		 *
		 * Ensures that derived classes can clean up resources properly when a task is destroyed.
		 */
		virtual ~ITask() = default;
	};


	/**
	* @brief Class representing a task in the Kernel.
	*
	* The Task class provides the implementation for a task that can be scheduled and executed by the Kernel.
	* It extends the ITask interface, providing additional functionality such as managing the task's stack,
	* priority, and state.
	*/
	class Task : public ITask
	{
	public:
	    /**
	     * @brief Constructs a Task object with a specified stack.
	     *
	     * Initializes the task with a given stack memory and size.
	     *
	     * @param stack Pointer to the stack memory allocated for the task.
	     * @param size The size of the stack memory in bytes.
	     */
	    Task(std::uint8_t* stack, std::size_t size);

	    /**
	     * @brief Constructs a Task object with default settings.
	     *
	     * Initializes the task without specifying stack memory. The stack can be set later using setStack().
	     */
	    Task();

	    /**
	     * @brief Destructor for the Task class.
	     *
	     * Cleans up any resources associated with the task.
	     */
	    ~Task() override;

	    /**
	     * @brief Sets the stack memory for the task.
	     *
	     * This method allows the stack memory and its size to be specified after the Task object has been created.
	     *
	     * @param stack Pointer to the stack memory allocated for the task.
	     * @param size The size of the stack memory in bytes.
	     */
	    void setStack(std::uint8_t* stack, std::size_t size);

	    /**
	     * @brief Puts the task to sleep for a specified number of ticks.
	     *
	     * This method suspends the task's execution for the given number of ticks, after which it will be resumed.
	     *
	     * @param ticks The number of ticks for which the task should sleep.
	     */
	    void sleep(std::uint32_t ticks);

	    /**
	     * @brief Yields the processor to allow another task to run.
	     *
	     * This method voluntarily relinquishes the CPU, allowing the Kernel to schedule another task.
	     */
	    void yield();

	    /**
	     * @brief Sets the priority of the task.
	     *
	     * Adjusts the task's priority level, which affects its scheduling.
	     *
	     * @param priority The new priority level for the task.
	     */
	    inline void setPriority(TaskPriority priority)
	    {
	        data.setPriority(priority);
	    }

	    /**
	     * @brief Sets the name of the task.
	     *
	     * Assigns a name to the task, which can be useful for debugging and tracking.
	     *
	     * @param name The name to assign to the task.
	     */
	    inline void setName(const std::string_view& name)
	    {
	        data.setName(name);
	    }

	    /**
	     * @brief Retrieves the task's internal data structure.
	     *
	     * Provides access to the TaskData object that contains the task's state, priority, and other attributes.
	     *
	     * @return Pointer to the TaskData object associated with this task.
	     */
	    inline TaskData* getTaskData()
	    {
	        return &data;
	    }

    protected:

		/**
		* @brief The task's main functionality.
		*
		* This method must be overridden by derived classes to define the task's behavior. It is called by the Kernel when the task is scheduled to run.
		*/
        void run() override
		{			
		}

    private:

        TaskData data;
        std::uint8_t* ptrStack = nullptr;
    };
}

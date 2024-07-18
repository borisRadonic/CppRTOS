#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <assert.h>
#include <functional>
#include "Port.hpp"
#include "Config.hpp"
#include "Task.hpp"
#include "Timer.hpp"
#include "IdleTask.hpp"
#include "Fifo.hpp"

namespace CppRtos
{

	constexpr std::uint32_t WAIT_FOREVER = 0xFFFFFFFF;
	
	enum class KernelState : std::uint8_t
	{
		eReset = 0u,
		eReady = 1u,
		eRunning = 2u,
		eLocked = 3u,
		eSuspended = 4u,
		eSleeping = 5u,
		eError = 0xFFu
	};


	enum class KernelError : std::uint32_t
	{
		eOK 			= 0u,
		eNotReady 		= 1u,
		eInInterrupt 	= 2u,
		eNotRunning 	= 3u,
		eLocked 		= 4u
	};
    
	

    class Kernel final
    {
		friend class KernelFactory;
    private:

		Kernel();

		~Kernel()
		{
		}

		 Kernel(const Kernel&) = delete;
		        Kernel& operator=(const Kernel&) = delete;
		        Kernel(Kernel&&) = delete;
		        Kernel& operator=(Kernel&&) = delete;

		void updateTimers();

	public:

		void addTask( Task& task  )
		{			
			if( taskCount < Settings::MAX_TASKS )
			{
				TaskData* ptrTaskData = task.getTaskData();
				ptrTaskData->setId( taskCount );
				tasks[taskCount] = ptrTaskData;
				taskCount++;
				
				assert( taskCount <= Settings::MAX_TASKS );
				//add task to the Ready List
				port.enterCritical();

				TaskPriority priority = ptrTaskData->getPriority();

				if( priority > highestTaskPriority )
				{
					highestTaskPriority = priority;
				}
							
				StackAddr pStack = port.initialiseStack(static_cast<void*>(ptrTaskData->getCurrentStackPtr()), static_cast<void*>(this ));
				ptrTaskData->setCurrentStackPtr( pStack );

				std::size_t prio = static_cast<std::size_t>(priority);
				assert( prio < readyTasks.size() );
				if( prio < readyTasks.size() )
				{
					readyTasks[prio] = readyTasks[prio] | (1u << ptrTaskData->getId());
				}
				port.exitCritical();
			}
		}
		
		void addSleepingTask(TaskData* task, std::uint32_t ticks);

		//this function is called only after entering critical section
		void setTaskReady( CppRtos::TaskData * ptrTask );

		bool addTimer(Timer* timer);

        void removeTimer(Timer* timer);		

		inline TaskData* getCurrentTask() const
		{
			return currentTask;
		}

		inline TaskData* getTaskById( std::uint32_t id ) const
		{
			if( id < Settings::MAX_TASKS )
			{
				return tasks[id];
			}
			return nullptr;
		}
		
        void initialize()
		{
			state = KernelState::eReady;
		}

		void start();


        void lock()
		{
			//todo
			state = KernelState::eLocked;
		}

        void unLock()
		{
			//todo
			//_state = KernelState::eRunning; ???
		}

        void suspend()
		{
		//todo
			state = KernelState::eSuspended;
		}

        inline void incrementTickCount()
		{
			port.incrementTickCount();
		}

        inline void incrementSysTimerCount()
		{
			port.incrementSysTimerCount();
		}

		inline std::uint64_t getTickCount() const
		{
			return port.getTickCount();
		}

		inline std::uint64_t getSysTimerCount() const
		{
			return port.getSysTimerCount();
		}

		inline void disableInterrupts()
		{
			port.disableInterrupts();
		}

		inline void enableInterrupts()
		{
			port.enableInterrupts();
		}

		inline void enterCritical(void)
		{
			port.enterCritical();
		}

		inline void exitCritical(void)
		{
			port.exitCritical();
		}

		inline void yield()
		{
			port.yield();
		}
		
		inline bool isInsideInterrupt( void ) const
		{
			return port.isInsideInterrupt();
		}

		void selectHighestPriorityTask();

		void tick();
		
		
    private:

        KernelState 	state = KernelState::eReset;

        KernelError 	lastError = KernelError::eOK;
      
        std::array<TaskData*, Settings::MAX_TASKS> tasks;
		
		//For max. MAX_PRIORY_LEVELS and max. 32 tasks
		std::array<std::uint32_t, MAX_PRIORY_LEVELS> readyTasks = {};

		std::uint32_t priorityBitmap = 0u; // Bitmap to track non-empty priority levels

		TaskData* currentTask = nullptr;

		TaskPriority highestTaskPriority = TaskPriority::PRIORITY_IDLE;
	
		std::size_t taskCount = 0u;  // Current count of added tasks

        Port::Port port;

		IdleTask idleTask;

		std::uint64_t sleepingTasksBitmap = 0u;
        
		std::array<std::uint64_t,Settings::MAX_TASKS> taskWakeUpTimes = {0u};

		std::array<Timer*, Settings::MAX_TIMERS> timers = {nullptr};

    };
}

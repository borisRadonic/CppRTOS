#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <assert.h>
#include <functional>

#include "Task.hpp"
#include "Port.hpp"
#include "Config.hpp"
#include "IdleTask.h"
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

		Kernel()
            :  _state( KernelState::eReset )
			, _lastError( KernelError::eOK )			
			, _taskCount (0u)
        	{
			_tasks.fill(nullptr);
			
			this->addTask( _idleTask );
			_currentTask = _idleTask.getTaskData();

			//Add Timer Task
        	}

		~Kernel()
		{
		}

		 Kernel(const Kernel&) = delete;
		        Kernel& operator=(const Kernel&) = delete;
		        Kernel(Kernel&&) = delete;
		        Kernel& operator=(Kernel&&) = delete;


    public:
	        template<std::size_t STACK_SIZE>
	        void addTask( Task<STACK_SIZE>& task  )
	        {
	        	if( _taskCount < Settings::MAX_TASKS )
	        	{
	        		TaskData* ptrTaskData = task.getTaskData();

	        		ptrTaskData->setId( _taskCount );
	        		_tasks[_taskCount] = ptrTaskData;
	        		_taskCount++;
	
	        		//add task to the Ready List
	        		_port.enterCritical();
								
					StackAddr pStack = _port.initialiseStack(static_cast<void*>(ptrTaskData->getCurrentStackPtr()), static_cast<void*>(this ));
					ptrTaskData->setCurrentStackPtr( pStack );

					std::size_t prio = static_cast<std::size_t>(ptrTaskData->getPriority());
					assert( prio < _readyTasks.size() );
					if( prio < _readyTasks.size() )
					{
						_readyTasks[prio].enqueue( ptrTaskData );
					}
					_port.exitCritical();
	        	}
	        }

		TaskData* getCurrentTask() const
		{
			return _currentTask;
		}
		
        void initialize()
		{
			_state = KernelState::eReady;
		}

		void start()
		{
			initialize();
			assert( _state == KernelState::eReady );
			if( _state == KernelState::eReady)
			{
				_port.startScheduler();
			}
		}


        void lock()
		{
			//todo
			_state = KernelState::eLocked;
		}

        void unLock()
		{
			//todo
			//_state = KernelState::eRunning; ???
		}

        	void suspend()
		{
		//todo
			_state = KernelState::eSuspended;
		}

        	inline void incrementTickCount()
		{
			_port.incrementTickCount();
		}

        	inline void incrementSysTimerCount()
		{
			_port.incrementSysTimerCount();
		}

		inline std::uint64_t getTickCount() const
		{
			return _port.getTickCount();
		}

		inline std::uint64_t getSysTimerCount() const
		{
			return _port.getSysTimerCount();
		}

		inline void disableInterrupts()
		{
			_port.disableInterrupts();
		}

		inline void enableInterrupts()
		{
			_port.enableInterrupts();
		}

		inline void enterCritical(void)
		{
			_port.enterCritical();
		}

		inline void exitCritical(void)
		{
			_port.exitCritical();
		}

		inline void yield()
		{
			_port.yield();
		}
		
		inline bool isInsideInterrupt( void ) const
		{
			return _port.isInsideInterrupt();
		}

		inline void selectHighestPriorityTask() 
		{
			/*
			static int task = 0;
			if( task == 0 )
			{
				task = 1;
			}
			else if( task == 1 )
			{
				task = 2;
			}
			else
			{
				task = 0;
			}
			_currentTask = _tasks[task];
			return;
			*/

			TaskPriority currentPriority = _currentTask->getPriority();
			//take next ready task with highest priority
			for (auto& task : _readyTasks)			
			{
				if( !task.isEmpty() )
				{
					TaskData* ptrTaskData = task.getAt(0);
					/*check condition for preemption*/
					if( _currentTask->getState() == TaskStateType::eRunning )
					{
						if( ptrTaskData->getPriority() >= currentPriority )
						{	
							TaskData* ptrTaskData = task.dequeue();
							_currentTask = ptrTaskData;
							//change the state of old Running task to Ready
							ptrTaskData->setState( TaskStateType::eReady );
							//add old task to list of ready tasks
							std::size_t prio = static_cast<std::size_t>(currentPriority);
							assert( prio < _readyTasks.size() );
							if( prio < _readyTasks.size() )
							{
								_readyTasks[prio].enqueue( ptrTaskData );
								break;
							}
						}
					}
					else
					{
						_currentTask = task.dequeue();
						_currentTask->setState( TaskStateType::eRunning );
						break;
					}
				}
			}

		}

		using ReadyTasks = Fifo<TaskData*, Settings::MAX_TASKS>;

    private:

        KernelState 	_state = KernelState::eReset;

        KernelError 	_lastError = KernelError::eOK;
      
        std::array<TaskData*, Settings::MAX_TASKS> _tasks;
		
		std::array< ReadyTasks, MAX_PRIORY_LEVELS> _readyTasks = {};

		TaskData* _currentTask = nullptr;

		IdleTask _idleTask;

				
		std::size_t _taskCount = 0u;  // Current count of added tasks

        Port::Port _port;

    };




	class KernelFactory
	{
	private:

		bool _isCreated = false;
		Kernel* kernel = nullptr;

		/**
		* @brief Private constructor to prevent external instantiation.
		*/
		KernelFactory() : _isCreated(false), kernel(nullptr)
		{
		}

	public:

		// Prevent creating multiple instances of the KernelFactory
		KernelFactory(const KernelFactory&) = delete;
		KernelFactory& operator=(const KernelFactory&) = delete;

		inline static KernelFactory& getInstance() noexcept
		{
			static KernelFactory instanceFactory; // This creates a single instance on first use
			return instanceFactory;
		}

		inline Kernel* getKernel() noexcept
		{
			return kernel;
		}

		Kernel* create( void* platformMemory ) noexcept
		{
			if (_isCreated)
			{
				return nullptr;
			}
			_isCreated = true;
			kernel = new(platformMemory) Kernel();
			return kernel;
		}

		void destroy( void* platformMemory )
		{
			if (_isCreated)
			{
				static_cast<Kernel*>(platformMemory)->~Kernel();
				_isCreated = false;
			}
		}
	};
}

#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <assert.h>

#include "Task.hpp"
#include "Port.hpp"
#include "Config.hpp"
#include "IdleTask.h"
#include "Fifo.hpp"

namespace CppRtos
{

	std::uint32_t WAIT_FOREVER = 0xFFFFFFFF;
	
	enum class KernelState : std::uint8_t
	{
		eReset = 0u,
		eReady = 1u,
		eRunning = 2u,
		eLocked = 3u,
		eSuspended = 4u,
		eError = 0xFFu
	};

	const char* KernelVersion = "1.1";


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
	        void addTask( Task<STACK_SIZE>& task )
	        {
	        	if( _taskCount < Settings::MAX_TASKS )
	        	{
	        		TaskData* ptrTaskData = task.getTaskData();
	        		ptrTaskData->setId( _taskCount );
	        		_tasks[_taskCount] = ptrTaskData;
	        		_taskCount++;
	
	        		//add task to the Ready List
	        		_port.enterCritical();
	
					_readyTasks.enqueue( ptrTaskData );
	
	        		_port.exitCritical();
	
	        	}
	        }

		TaskData* getCurrentTask() const
		{
			return _currentTask;
		}

	        const char* getVersion()
	        {
	        	return KernelVersion;
	        }

        	void initialize()
		{
			_state = KernelState::eReady;
		}

		void start()
		{
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

		inline void yield()
		{
			_port.yield();
		}

		inline void switchContext()
		{
			/*TODO: Implement logic here...*/
			_port.switchContext();
		}

		inline bool isInsideInterrupt( void ) const
		{
			return _port.isInsideInterrupt();
		}

		inline void selectHighestPriorityTask()
		{
			//todo
		}


    private:

        KernelState 	_state = KernelState::eReset;

        KernelError 	_lastError = KernelError::eOK;
      
        std::array<TaskData*, Settings::MAX_TASKS> _tasks;

	Fifo<TaskData*, Settings::MAX_TASKS> _readyTasks = {};

	TaskData* _currentTask = nullptr;

	IdleTask _idleTask;

        std::size_t _taskCount = 0u;  // Current count of added tasks

        Port::Port _port;

    };


    std::aligned_storage_t<sizeof(Kernel)> buffer;

	class KernelFactory
	{
	private:

		bool _isCreated = false;
		Kernel* _instance = nullptr;

		/**
		* @brief Private constructor to prevent external instantiation.
		*/
		KernelFactory() : _isCreated(false), _instance(nullptr)
		{
		}

	public:

		// Prevent creating multiple instances of the KernelFactory
		KernelFactory(const KernelFactory&) = delete;
		KernelFactory& operator=(const KernelFactory&) = delete;

		inline static KernelFactory& getInstance()
		{
			static KernelFactory instanceFactory; // This creates a single instance on first use
			return instanceFactory;
		}

		inline Kernel* getKernel()
		{
			return _instance;
		}

		Kernel* create( void* platformMemory )
		{
			if (_isCreated)
			{
				return nullptr;
			}
			_isCreated = true;
			return new(platformMemory) Kernel();
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

#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "Task.hpp"
#include "Port.hpp"
#include "Config.hpp"

namespace CppRtos
{

	constexpr std::uint32_t MAX_NUMBER_OF_TASKS = 16u;

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

	//using StackAddr = std::size_t;
       

    class Kernel final
    {
    public:

		Kernel()
            :  _state( KernelState::eReset )
			, _lastError( KernelError::eOK )
			, _tickCount ( 0u )
			, _sysTimerCount( 0u )
			, _taskCount (0u)
        {
			_tasks.fill(nullptr);
        }

        ~Kernel() = default;

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

        		_port.exitCritical();

        	}
        }

        const char* getVersion()
        {
        	return KernelVersion;
        }

        bool initialize();

        void lock();

        void unLock();

        void suspend();

        void incrementTickCount();

        void incrementSysTimerCount();

        inline std::uint64_t getTickCount() const
        {
        	return _tickCount;
        }

        inline std::uint64_t getSysTimerCount() const
        {
        	return _sysTimerCount;
        }

    private:


        KernelState 	_state = KernelState::eReset;

        KernelError 	_lastError = KernelError::eOK;

        std::uint64_t 	_tickCount = 0u;

        std::uint64_t 	_sysTimerCount = 0u;

        std::array<TaskData*, Settings::MAX_TASKS> _tasks;

        std::size_t _taskCount = 0u;  // Current count of added tasks

        Port::Port _port;

    };
}

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
		friend class KernelFactory;
    private:

		Kernel()
            :  _state( KernelState::eReset )
			, _lastError( KernelError::eOK )
			, _tickCount ( 0u )
			, _sysTimerCount( 0u )
			, _taskCount (0u)
        {
			_tasks.fill(nullptr);
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

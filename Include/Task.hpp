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

	enum class TaskStateType : std::uint8_t
	{
		eReady			= 0u,
		eRunning		= 1u,
		eBlocked		= 2u,
		eSuspended		= 3u
	};

	enum class TaskPriority : uint8_t
	{
		PRIORITY_HIGHEST 			= 0u,    // Highest priority
		PRIORITY_VERY_HIGH 			= 1u,
		PRIORITY_HIGH 				= 2u,
		PRIORITY_MODERATELY_HIGH 	= 3u,
		PRIORITY_MEDIUM_HIGH 		= 4u,
		PRIORITY_SLIGHTLY_HIGH 		= 5u,
		PRIORITY_ABOVE_AVERAGE 		= 6u,
		PRIORITY_AVERAGE 			= 7u,
		PRIORITY_BELOW_AVERAGE 		= 8u,
		PRIORITY_SLIGHTLY_LOW 		= 9u,
		PRIORITY_MEDIUM_LOW 		= 10u,
		PRIORITY_MODERATELY_LOW 	= 11u,
		PRIORITY_LOW 				= 12u,
		PRIORITY_VERY_LOW 			= 13u,
		PRIORITY_LOWER_LOW 			= 14u,
		PRIORITY_IDLE				= 15u,	// Lowest priority
		PRIORITY_RESERVED           = 0xFF
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
       
	class TaskData
	{

	 public:

		explicit TaskData()
		: _currentStackPointer( nullptr )
		, _priority( TaskPriority::PRIORITY_IDLE )
		, _basePriority( TaskPriority::PRIORITY_IDLE )
		, _numOfHeldMutexes( 0u )
		, _runCounter( 0u )
		, _state(TaskStateType::eReady)
		, _id( 0u )
		, _startStack (0u)
		, _endStack( 0u )
	    {
			_name.fill(0);
	    }

		~TaskData()
	    {			
	    }

		inline void resetPriority()
		{
			_priority = _basePriority;
		}


		inline void setTopStack( const StackAddr topStack)
		{
			_endStack = topStack;
			_currentStackPointer = topStack; //at start
		}

		inline StackAddr getCurrentStackPtr() const
		{
			return _currentStackPointer;
		}

		inline void setStartStack( const StackAddr startStack)
		{
			_startStack = startStack;
		}

		inline void setCurrentStackPtr( const StackAddr currentStack )
		{
			_currentStackPointer = currentStack;
		}
		

		inline StackAddr getStartStackddress() const
		{
			return _startStack;
		}

		inline void setPriority(TaskPriority priority)
		{
			if( priority > TaskPriority::PRIORITY_IDLE )
			{
				_priority = TaskPriority::PRIORITY_IDLE;
			}
			else
			{
				_priority = priority;
			}
		}

		inline TaskPriority getPriority() const
		{
			return _priority;
		}

		inline void setBasePriority( const TaskPriority basePriority )
		{
			_basePriority = basePriority;
		}

		inline TaskPriority getBasePriority() const
		{
			return _basePriority;
		}

		inline void setTaskInterfacePtr( ITask* taskIntPtr )
		{
			taskInterfacePtr = taskIntPtr;
		}

		inline ITask* getTaskInterfacePtr() const 
		{
			return taskInterfacePtr;
		}

		inline StackAddr getEndStackddress() const
		{
			return _endStack;
		}

		inline void setState( const TaskStateType state )
		{
			_state = state;
		}

		inline TaskStateType getState() const
		{
			return _state;
		}

		inline void setName( const std::string_view&  name )
		{
			_name.fill(0);
			size_t num_to_copy = std::min(name.size(), _name.size() - 1);
			std::copy_n(name.begin(), num_to_copy, _name.begin());
		}

		inline void setId( std::uint32_t id )
		{
			_id = id;
		}

		inline std::uint32_t getId() const
		{
			return _id;
		}

	 protected:
		StackAddr 		_currentStackPointer = 0u; 
		TaskPriority 	_priority = TaskPriority::PRIORITY_IDLE;

        TaskPriority 	_basePriority = TaskPriority::PRIORITY_IDLE; // Used for mutexes by priority inheritance logic
        std::uint32_t 	_numOfHeldMutexes = 0u;

        std::uint64_t 	_runCounter = 0U;


        TaskStateType 	_state 	= TaskStateType::eReady;

        std::uint32_t	_id 	= 0u;

        std::array<char, MAX_TASK_NAME> _name 	= {};

        StackAddr _startStack = 0u; ///  Start of the stack
        StackAddr _endStack = 0u; /// End of the stack  ( ONly for recording purposes )

		 ITask* taskInterfacePtr = nullptr;

	};

	class ITask
	 {
		public:
    		virtual void run() = 0; 
    		virtual ~ITask() = default;
	};


    template<std::size_t STACK_SIZE>
    class Task : public ITask
    {
    public:

    	Task()
        {
        	_stack.fill (0 );
        	std::size_t start =  reinterpret_cast<std::size_t>( _stack.data() );
        	_data.setStartStack( _stack.data() );
        	_data.setTopStack( reinterpret_cast<void*>( start + _stack.size() ));
			_data.setTaskInterfacePtr( static_cast<ITask*>(this) );
        }

        virtual ~Task()
        {
        }

        inline void suspend()
        {
        	_data.setState( TaskStateType::eSuspended );
        }

        inline void resume()
        {
        	_data.setState( TaskStateType::eReady );
        }

        inline void block()
        {
        	_data.setState( TaskStateType::eBlocked );
        }

        inline void unblock()
        {
        	_data.setState( TaskStateType::eReady );
        }

        inline void setPriority( TaskPriority priority )
        {
        	_data.setPriority( priority );
        }

        inline void setName( const std::string_view&  name )
        {
        	_data.setName( name );
        }


        inline TaskData* getTaskData()
        {
        	return &_data;
        }

        virtual void run()
		{			
		}

    private:

        TaskData _data;
        std::array<std::uint8_t, STACK_SIZE> _stack = {};
    };
}

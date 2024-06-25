#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>

#include "Config.hpp"

namespace CppRtos
{

	constexpr std::uint32_t MAX_TASK_NAME = 16u;

	enum class TaskStateType : std::uint8_t
	{
		eReady			= 0u,
		eRunning		= 1u,
		eBlocked		= 2u,
		eSuspended		= 3u
	};


	using StackAddr = std::size_t;
       



	class TaskData
	{

	 public:

		explicit TaskData()
		: _topStack( 0u )
		, _priority(0u)
		, _basePriority (0u)
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


		inline void setTopStack( const StackAddr topStack)
		{
			_topStack = topStack;
			_endStack = _topStack;
		}

		inline StackAddr getTopStackAddress() const
		{
			return _topStack;
		}

		inline void setStartStack( const StackAddr startStack)
		{
			_startStack = startStack;
		}

		inline StackAddr getStartStackddress() const
		{
			return _startStack;
		}

		inline void setPriority(const std::uint32_t priority)
		{
			if( priority >= Settings::MAX_PRIORITY )
			{
				_priority = Settings::MAX_PRIORITY -1u;
			}
			else
			{
				_priority = priority;
			}
		}

		inline int getPriority() const
		{
			return _priority;
		}


		inline void setBasePriority( const std::uint32_t basePriority )
		{
			_basePriority = basePriority;
		}

		inline int getBasePriority() const
		{
			return _basePriority;
		}


		inline void setRecordEndStackAddress()
		{
			_endStack = _topStack;
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
		StackAddr 		_topStack = 0u; /// Points to the top of the stack
		std::uint32_t 	_priority = 0u;

        std::uint32_t 	_basePriority = 0u; // Used for mutexes by priority inheritance logic
        std::uint32_t 	_numOfHeldMutexes = 0u;

        std::uint64_t 	_runCounter = 0U;


        TaskStateType 	_state 	= TaskStateType::eReady;

        std::uint32_t	_id 	= 0u;

        std::array<char, MAX_TASK_NAME> _name 	= {};

        StackAddr _startStack = 0u; ///  Start of the stack
        StackAddr _endStack = 0u; /// End of the stack  ( ONly for recording purposes )


	};

    template<std::size_t STACK_SIZE>
    class Task
    {
    public:

    	Task()
        {
        	_stack.fill (0 );
        	std::size_t start =  reinterpret_cast<std::size_t>( _stack.data() );
        	_data.setStartStack( start );
        	_data.setTopStack( start + _stack.size() );
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

        inline void setPriority( std::uint32_t priority )
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

        virtual void run() = 0;

    private:

        TaskData _data;
        std::array<std::uint8_t, STACK_SIZE> _stack = {};
    };
}

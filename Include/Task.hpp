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

 #include <atomic>

    template<typename T, std::size_t MAX_ITEMS>
    class Fifo
    {
    public:
        Fifo() : _head(0), _tail(0), _size(0)
        {
        }

        inline void enqueue(const T& item)
        {
            if (_size < MAX_ITEMS)
            {
        	    _buffer[_tail] = item;
	            _tail = (_tail + 1) % MAX_ITEMS;
        	    ++_size;
	        }
        }

        inline T dequeue()
        {
            if (size > 0)
 	        {
        	    T item = buffer[_head];
	            _head = (_head + 1) % MAX_ITEMS;
        	    --size;
        	    return item;
            }
		    return nullptr;
        }

        inline bool isEmpty() const
        {
            return (_size == 0u);
        }

        inline bool isFull() const
        {
            return (_size == MAX_ITEMS);
        }

        std::size_t getSize() const
        {
            return _size;
        }
    private:
        std::array<T, MAX_ITEMS> _buffer = {};
        std::size_t _head = 0u;
        std::size_t _tail = 0u;
        std::size_t _size = 0u;
    };


    class Semaphore
    {
    public:
        
        Semaphore() : count (0)
        {

        }

        Semaphore(int initialCount ) : count(initialCount)
        {
        }

         /*Semaphore can be acquired from any task but not from interrupt!*/
        void acquire()
        {
            //assert(not_from_interrupt);
            //if in_interrupt
            //{
            //return;
            //}

            while (true)
            {
                // Spin-wait until the count is greater than zero
                while (count.load() == 0)
                {
                    if( !_blockedTasks.isFull() )
                    {
                        //disable interrupts
                        //get current task
                        Task* thisTask = Kernel::getCurrentTask();
                        thisTask->setBlocked();
                        _blockedTasks.enqueue(thisTask);
                        //enable interrupts
                        //switch context
                        Scaduler::switchContext();
                    }
                }

                // Attempt to decrement the count
                int expected = count.load();
                if (count.compare_exchange_weak(expected, expected - 1))
                {
                    break; // Successfully acquired the semaphore
                }
            }
        }

        /*Semaphore can be released from interrupt and from any task*/
        void release()
        {	
            //disable interrupts
            removeNonBlocked();
            count.fetch_add(1);
            // Check which blocked task has to be set to ready
            if( false == _blockedTasks.isEmpty() )
            {
                Task* pBlockedTask = _blockedTasks.dequeue();
                pBlockedTask->setReady();
            }
            //enable interrupts
        }

    private:
        std::atomic<int> count;
        Fifo<Task*,MAX_TASKS>_blockedTasks;

        void removeNonBlocked()
        {
            for (size_t i = 0; i < _blockedTasks.getSize(); ++i)
            {
                if( _blockedTasks[i]->getStatus != eBlocked )
                {
                    /*remove task from the list*/
                    RemoveAt(i);
                }
            }
        }
    };

    class Mutex
    {
    public:
        Mutex( /*add options*/) : owner(nullptr), count(1)
        {        
        }

        void acquire(int timeout /*todo:timeout*/)
        {
            Task* currentTask = Kernel::getCurrentTask();
            while (true)
            {
                while (count.load() == 0)
                {
                    if (!_blockedTasks.isFull())
                    {
                        // Disable interrupts
                        disableInterrupts();

                        removeNonBlocked();

                        currentTask->setBlocked();
                        _blockedTasks.enqueue(currentTask);
                        if (owner && owner->getPriority() < currentTask->getPriority())
                        {
                            owner->setPriority(currentTask->getPriority());
                        }

                        // Enable interrupts
                        enableInterrupts();

                        // Switch context
                        Scheduler::switchContext();
                    }
                }

                // Attempt to decrement the count and acquire the mutex
                int expected = count.load();
                if (count.compare_exchange_weak(expected, expected - 1))
                {
                    // Successfully acquired the mutex
                    owner = currentTask;
                    break;
                }
            }
        }

        void release()
        {
            Task* currentTask = Kernel::getCurrentTask();
            if (currentTask == owner)
            {
                // Disable interrupts
                disableInterrupts();

                /*Remove tasks which are not blocked */
               removeNonBlocked();

                count.fetch_add(1);
                owner = nullptr;
                if (!_blockedTasks.isEmpty())
                {
                    Task* unblockedTask = _blockedTasks.dequeue();
                    unblockedTask->setReady();
                    // determine if the current owner task should reset to its original priority
                    if (!isAnyTaskBlocking(unblockedTask->getPriority()))
                    {
                        currentTask->resetPriority();
                    }
                }
                // Enable interrupts
                enableInterrupts();
            }
        }

        //flush makes all tasks that are currently waiting for the semaphore ready to run,
        // but it does not change the state of the semaphore (it does not increase or decrease the semaphore's count).
        void flush()
        {
            disableInterrupts();

            /*Remove tasks which are not blocked */
            removeNonBlocked();

            while( false == _blockedTasks.isEmpty() )
            {
                Task* unblockedTask = _blockedTasks.dequeue();
                unblockedTask->setReady();   
            }
            // Enable interrupts
            enableInterrupts();
        }

    private:
        std::atomic<int> count = 0;
        Task* owner = nullptr;
        Fifo<Task*, MAX_TASKS> _blockedTasks;

        void removeNonBlocked()
        {
            for (size_t i = 0; i < _blockedTasks.getSize(); ++i)
            {
                if( _blockedTasks[i]->getStatus != eBlocked )
                {
                    /*remove task from the list*/
                    RemoveAt(i);
                }
            }
        }
 
        bool isAnyTaskBlocking(int priority)
        {
            // Check if any tasks in the blocked queue have the given priority
            for (size_t i = 0; i < _blockedTasks.getSize(); ++i)
            {               
                if (_blockedTasks[i]->getPriority() == priority)
                {
                    return true;
                }
            }
            return false;
        }
        void removeAt(std::size_t pos)
        {
            for (std::size_t i = pos; i != (tail - 1 + MAX_ITEMS) % MAX_ITEMS; i = (i + 1) % MAX_ITEMS)
            {
                std::size_t next = (i + 1) % MAX_ITEMS;
                buffer[i] = buffer[next];
            }
            tail = (tail - 1 + MAX_ITEMS) % MAX_ITEMS;
            size.fetch_sub(1);
        }
    };


    template<typename T, std::size_t MAX_ITEMS>
    class FifoQueue
    {
    public:
        FifoQueue() : head(0), tail(0), size(0)
        {            
        }

        // Adds an item to the queue
        void send(const T& item)
        {
            while (true)
            {
                // Wait if the queue is full
                if (size.load() < MAX_ITEMS)
                {
                    disableInterrupts();
                    if (size.load() < MAX_ITEMS) {
                        buffer[tail] = item;
                        tail = (tail + 1) % MAX_ITEMS;
                        size.fetch_add(1);
                        enableInterrupts();
                        break;
                    }
                    enableInterrupts();
                }
                // Optionally yield to other tasks
                yield();
            }
        }

        // Removes an item from the queue
        T receive( int timeout /*todo:timeout*/)
        {
            while (true)
            {
                // Wait if the queue is empty
                if (size.load() > 0)
                {
                    disableInterrupts();
                    if (size.load() > 0)
                    {
                        T item = buffer[head];
                        head = (head + 1) % MAX_ITEMS;
                        size.fetch_sub(1);
                        enableInterrupts();
                        return item;
                    }
                    enableInterrupts();
                }
                // Optionally yield to other tasks
                yield();
            }
        }

        // Checks if the queue is empty
        bool isEmpty() const
        {
            return size.load() == 0;
        }

        // Checks if the queue is full
        bool isFull() const
        {
            return size.load() == MAX_ITEMS;
        }

        // Returns the current size of the queue
        std::size_t getSize() const
        {
            return size.load();
        }

    private:
        std::array<T, MAX_ITEMS> buffer;
        std::atomic<std::size_t> head;
        std::atomic<std::size_t> tail;
        std::atomic<std::size_t> size;

        void disableInterrupts()
        {
            // Implementation to disable interrupts
        }

        void enableInterrupts()
        {
            // Implementation to enable interrupts
        }

        void yield()
        {
            // Implementation to yield the processor, if applicable
        }
    };




}

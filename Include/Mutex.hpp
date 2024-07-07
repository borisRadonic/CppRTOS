#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>
#include <atomic>
#include "Kernel.hpp"
#include "Config.hpp"
#include "Fifo.hpp"

namespace CppRtos
{
    enum class MutexResult : std::uint8_t
    {
        Success  = 0u,
        ErrorCalledFromISR  = 1u,
        Timeout  = 2u
    };

    class Mutex
    {
    public:

        Mutex( /*add options*/) : _owner(nullptr), _count(1)
        {        
            KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            _ptrKernel = ptrKernelFactory.getKernel();
        }

        MutexResult acquire(std::uint32_t ticksTimeout)
        {            
            assert( _ptrKernel->isInsideInterrupt() == false );
            if( !_ptrKernel->isInsideInterrupt())
            {
                TaskData* ptrTaskData = _ptrKernel->getCurrentTask();
            
                bool waitForever = (ticksTimeout == WAIT_FOREVER);
                std::uint64_t end = ticksTimeout + _ptrKernel->getTickCount();

                while (true)
                {
                    if( !waitForever )
                    {
                        if( _ptrKernel->getTickCount() > end)
                        {
                            return MutexResult::Timeout;
                        }
                    }
                    while (_count.load() == 0)
                    {
                        if (!_blockedTasks.isFull())
                        {
                            // Disable interrupts
                            _ptrKernel->disableInterrupts();
                        
                            removeNonBlocked();

                            ptrTaskData->setState( TaskStateType::eBlocked);
                            _blockedTasks.enqueue(ptrTaskData);
                            if ( (_owner != nullptr) && (_owner->getPriority() < ptrTaskData->getPriority() ) )
                            {
                                _owner->setPriority( ptrTaskData->getPriority() );
                            }

                            // Enable interrupts
                            _ptrKernel->enableInterrupts();

                            // Switch context
                            _ptrKernel->yield();
                        }
                    }

                    // Attempt to decrement the count and acquire the mutex
                    int expected = _count.load();
                    if (_count.compare_exchange_weak(expected, expected - 1))
                    {
                        // Successfully acquired the mutex
                        _owner = ptrTaskData;
                        break;
                    }
                }
                return MutexResult::Success;
            }
            return MutexResult::ErrorCalledFromISR;
        }

        MutexResult release()
        {
            assert( _ptrKernel->isInsideInterrupt() == false );
            if( !_ptrKernel->isInsideInterrupt())
            {
                TaskData* ptrTaskData = _ptrKernel->getCurrentTask();
                if (ptrTaskData == _owner)
                {
                    // Disable interrupts
                    _ptrKernel->disableInterrupts();

                    /*Remove tasks which are not blocked */
                    removeNonBlocked();

                    _count.fetch_add(1);
                    _owner = nullptr;
                    if (!_blockedTasks.isEmpty())
                    {
                        TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                        ptrUnblockedTask->setState( TaskStateType::eReady );
                        // determine if the current owner task should reset to its original priority
                        if (!isAnyTaskBlocking(ptrUnblockedTask->getPriority()))
                        {
                            ptrTaskData->resetPriority();
                        }
                    }
                    // Enable interrupts
                    _ptrKernel->enableInterrupts();
                }
                return MutexResult::Success;
            }
            return MutexResult::ErrorCalledFromISR;
        }       

    private:

        std::atomic<std::int32_t> _count = 0;

        TaskData* _owner = nullptr;

        Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks = {};

        Kernel* _ptrKernel = nullptr;


        void removeNonBlocked()
        {           
            for (size_t i = 0; i < _blockedTasks.getSize(); ++i)
            {
                TaskData* ptrTask = _blockedTasks.getAt( i );
                if( (ptrTask != nullptr) && (ptrTask->getState() != TaskStateType::eBlocked) )
                {
                    _blockedTasks.removeAt(i);
                }             
            }
        }
 
        bool isAnyTaskBlocking(int priority)
        {
            // Check if any tasks in the blocked queue have the given priority           
            for (size_t i = 0; i < _blockedTasks.getSize(); ++i)
            {   
                TaskData* ptrTask = _blockedTasks.getAt( i );            
                if (ptrTask->getPriority() == priority)
                {
                    return true;
                }
            }
            return false;
        }
       
    };
}

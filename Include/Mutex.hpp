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
#include <limits>

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

        Mutex() : _count(0), owner_id ( std::numeric_limits<std::uint32_t>::max() )
        {
        }

         ~Mutex()
        {
            // Ensure no tasks are blocked when the mutex is destroyed
            while (!_blockedTasks.isEmpty())
            {
                TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                if (ptrUnblockedTask != nullptr)
                {
                    ptrUnblockedTask->setState(TaskStateType::eReady);
                }
            }
        }

      
        MutexResult acquire(std::uint32_t ticksTimeout)
        {
            KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            Kernel* ptrKernel = ptrKernelFactory.getKernel();         
            assert( ptrKernel->isInsideInterrupt() == false );
            if( !ptrKernel->isInsideInterrupt())
            {
                TaskData* ptrTaskData = ptrKernel->getCurrentTask();
            
                bool waitForever = (ticksTimeout == WAIT_FOREVER);
                std::uint64_t end = ticksTimeout + ptrKernel->getTickCount();

                while (true)
                {
                    if( !waitForever )
                    {
                        if( ptrKernel->getTickCount() > end)
                        {
                            return MutexResult::Timeout;
                        }
                    }
                    std::int32_t expected = 0;  
                    if (_count.compare_exchange_strong(expected, 1))
                    {
                        // The exchange was successful, _count is now 1
                        owner_id = ptrTaskData->getId();
                        return MutexResult::Success;
                    }
                    else
                    {

                         std::int32_t is = _count.load();
                         is++;
                         /*
                            if ( _owner->getPriority() < ptrTaskData->getPriority() ) )
                            {
                                _owner->setPriority( ptrTaskData->getPriority() );
                            }
                            */
                         ptrKernel->enterCritical();
                        removeNonBlocked();
                        if (!_blockedTasks.isFull())
                        {                           
                            // The exchange failed, expected now holds the current value of _count
                            ptrTaskData->setState( TaskStateType::eBlocked );
                            _blockedTasks.enqueue(ptrTaskData);
                        }
                        // Switch context
                        ptrKernel->yield();
                        continue;
                    }
                }                
            }
            return MutexResult::ErrorCalledFromISR;
        }


        MutexResult release()
        {
            KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            Kernel* ptrKernel = ptrKernelFactory.getKernel();
            assert( ptrKernel->isInsideInterrupt() == false );
            if( !ptrKernel->isInsideInterrupt())
            {
                TaskData* ptrTaskData = ptrKernel->getCurrentTask();

                if (( ptrTaskData != nullptr) && (ptrTaskData->getId() == owner_id) )
                {
                    // Disable interrupts
                    ptrKernel->enterCritical();

                    /*Remove tasks which are not blocked */
                    //removeNonBlocked();

                    _count.fetch_add(1);
                    owner_id = std::numeric_limits<std:: uint32_t>::max();
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
                    // Switch context
                    ptrKernel->yield();

                    // Enable interrupts
                    ptrKernel->exitCritical();
                }
                return MutexResult::Success;
            }
            return MutexResult::ErrorCalledFromISR;
        }       

    private:

        std::atomic<std::int32_t> _count = 0;

        std::uint32_t owner_id = std::numeric_limits<std::uint32_t>::max();

        Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks = {};


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
 
        bool isAnyTaskBlocking(CppRtos::TaskPriority priority)
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

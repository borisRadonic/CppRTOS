#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>
#include "KernelFactory.hpp"
#include "Config.hpp"
#include "Fifo.hpp"
#include <limits>

namespace CppRtos
{
    enum class MutexResult : std::uint8_t
    {
        Success = 0u,
        ErrorCalledFromISR = 1u,
        Timeout = 2u,
        AlreadyOwned = 3u,
        NotOwner = 4u
    };

    class Mutex
    {
    public:

        Mutex() : count(0), owner ( nullptr )
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

            TaskData* ptrTaskData = ptrKernel->getCurrentTask();

            if( owner == ptrTaskData )
            {
                // Handle recursive mutex acquisition if needed
                return MutexResult::AlreadyOwned;
            }

            if( !ptrKernel->isInsideInterrupt())
            {                               
                bool waitForever = (ticksTimeout == WAIT_FOREVER);
                std::uint64_t end = ticksTimeout + ptrKernel->getTickCount();

                while (true)
                {
                    ptrKernel->enterCritical();                   
                    
                    if (count <= 0)
                    {
                        //todo:this can be done with std::atomic, but there are some problems at the moment
                        ptrKernel->enterCritical();
                        count++;
                        owner = ptrTaskData;
                        ptrKernel->exitCritical();
                        return MutexResult::Success;
                    }
                    else
                    {
                        if (!waitForever && ptrKernel->getTickCount() > end)
                        {
                            ptrKernel->exitCritical();
                            return MutexResult::Timeout;
                        }

                        if( owner->getPriority() < ptrTaskData->getPriority() )
                        {
                            owner->setPriority( ptrTaskData->getPriority() );
                        }
                      
                         
                        if (!_blockedTasks.isFull())
                        {                           
                            // The exchange failed, expected now holds the current value of _count
                            ptrTaskData->setState( TaskStateType::eBlocked );
                            _blockedTasks.enqueue(ptrTaskData);
                        }

                        ptrKernel->exitCritical();
                        
                        // Switch context
                        ptrKernel->yield();
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

            if( owner == nullptr )
            {
                return MutexResult::Success;
            }

            if( !ptrKernel->isInsideInterrupt())
            {
                TaskData* ptrTaskData = ptrKernel->getCurrentTask();

                if (( ptrTaskData != nullptr) && (ptrTaskData == owner) )
                {
                    // Disable interrupts
                    ptrKernel->enterCritical();
                    
                    count--;
                    owner = nullptr;

                    if (!_blockedTasks.isEmpty())
                    {
                        TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                        ptrKernel->setTaskReady(ptrUnblockedTask);

                        // Restore the original priority if needed
                        if (!isAnyTaskBlocking(ptrUnblockedTask->getPriority()))
                        {
                            ptrTaskData->resetPriority();
                        }
                        
                    }
                    ptrKernel->exitCritical();
                    // Switch context
                    ptrKernel->yield();
                    return MutexResult::Success;
                }
                return MutexResult::NotOwner;
            }
            return MutexResult::ErrorCalledFromISR;
        }


    private:

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
       
       private:
            std::int32_t count = 0;
            TaskData* owner = nullptr;
            Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks = {};
    };
}

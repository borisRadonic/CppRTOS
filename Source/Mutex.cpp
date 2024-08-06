#include "Mutex.hpp"
#include "KernelFactory.hpp"

namespace CppRtos
{

    Mutex::Mutex()
    : count(0)
    , owner ( nullptr )
    {
    }

    Mutex::~Mutex()
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

    MutexResult Mutex::acquire(std::uint32_t ticksTimeout)
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
                        if (ptrTaskData->getState() == TaskStateType::eRunning)
                        {
                            ptrKernel->resetTaskReady(ptrTaskData, TaskStateType::eBlocked);
                        }
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

    MutexResult Mutex::release()
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

    bool Mutex::isAnyTaskBlocking(CppRtos::TaskPriority priority)
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
}
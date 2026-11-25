#include "Mutex.hpp"
#include "KernelFactory.hpp"

namespace CppRtos
{
    Mutex::Mutex()
    : owner ( nullptr )
    {
    }

    Mutex::~Mutex()
    {
        // Ensure no tasks stay blocked when the mutex is destroyed: they return with a timeout
        if (!waitingTasks.isEmpty())
        {
            Kernel* ptrKernel = KernelFactory::getInstance().getKernel();
            if (ptrKernel != nullptr)
            {
                ptrKernel->enterCritical();
                static_cast<void>(ptrKernel->wakeAll(waitingTasks, WakeReason::eTimeout));
                ptrKernel->exitCritical();
            }
        }
    }

    MutexResult Mutex::acquire(std::uint32_t ticksTimeout)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();
        assert( ptrKernel->isInsideInterrupt() == false );

        if( ptrKernel->isInsideInterrupt() )
        {
            return MutexResult::ErrorCalledFromISR;
        }

        ptrKernel->enterCritical();
        TaskData* ptrTaskData = ptrKernel->getCurrentTask();

        if( owner == nullptr )
        {
            owner = ptrTaskData;
            ptrTaskData->incrementHeldMutexCount();
            ptrKernel->exitCritical();
            return MutexResult::Success;
        }

        if( owner == ptrTaskData )
        {
            ptrKernel->exitCritical();
            return MutexResult::AlreadyOwned;
        }

        if( ticksTimeout == 0u )
        {
            ptrKernel->exitCritical();
            return MutexResult::Timeout;
        }

        // Priority inheritance: the owner runs at least at the priority of the waiting task
        if( isHigherPriority( ptrTaskData->getPriority(), owner->getPriority() ) )
        {
            ptrKernel->changeTaskPriority( owner, ptrTaskData->getPriority() );
        }

        ptrKernel->blockCurrentTask( waitingTasks, ticksTimeout );
        ptrKernel->exitCritical();
        ptrKernel->yield();

        // Resumed: ownership was handed over by release(), or the wait timed out
        ptrKernel->enterCritical();
        const WakeReason reason = ptrTaskData->getWakeReason();
        if( (reason != WakeReason::eSignaled) && (owner != nullptr) && (owner->getHeldMutexCount() == 1u) )
        {
            // This waiter is gone: the owner keeps only the priority still required by the remaining waiters.
            // If the owner holds more than one mutex, the inherited priority is kept until it releases them.
            TaskPriority target = owner->getBasePriority();
            const TaskPriority highestWaiter = ptrKernel->getHighestWaiterPriority( waitingTasks );
            if( isHigherPriority( highestWaiter, target ) )
            {
                target = highestWaiter;
            }
            ptrKernel->changeTaskPriority( owner, target );
        }
        ptrKernel->exitCritical();

        return (reason == WakeReason::eSignaled) ? MutexResult::Success : MutexResult::Timeout;
    }

    MutexResult Mutex::release()
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();
        assert( ptrKernel->isInsideInterrupt() == false );

        if( ptrKernel->isInsideInterrupt() )
        {
            return MutexResult::ErrorCalledFromISR;
        }

        ptrKernel->enterCritical();

        if( owner == nullptr )
        {
            ptrKernel->exitCritical();
            return MutexResult::Success;
        }

        TaskData* ptrTaskData = ptrKernel->getCurrentTask();
        if( ptrTaskData != owner )
        {
            ptrKernel->exitCritical();
            return MutexResult::NotOwner;
        }

        // Undo priority inheritance once the task owns no mutex anymore
        ptrTaskData->decrementHeldMutexCount();
        if( ptrTaskData->getHeldMutexCount() == 0u )
        {
            ptrKernel->changeTaskPriority( ptrTaskData, ptrTaskData->getBasePriority() );
        }

        // Hand ownership over to the highest-priority waiter
        bool switchRequired = false;
        TaskData* ptrNewOwner = ptrKernel->wakeOne( waitingTasks, WakeReason::eSignaled, switchRequired );
        owner = ptrNewOwner;
        if( ptrNewOwner != nullptr )
        {
            ptrNewOwner->incrementHeldMutexCount();

            // The new owner inherits the priority of the remaining waiters
            const TaskPriority highestWaiter = ptrKernel->getHighestWaiterPriority( waitingTasks );
            if( !waitingTasks.isEmpty() && isHigherPriority( highestWaiter, ptrNewOwner->getPriority() ) )
            {
                ptrKernel->changeTaskPriority( ptrNewOwner, highestWaiter );
            }
        }
        ptrKernel->exitCritical();

        if( switchRequired )
        {
            ptrKernel->yield();
        }
        return MutexResult::Success;
    }
}

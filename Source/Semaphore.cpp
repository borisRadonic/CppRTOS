#include "Semaphore.hpp"
#include "KernelFactory.hpp"

namespace CppRtos
{
    Semaphore::Semaphore(std::uint32_t maxCount, std::uint32_t initialCount )
    : maxCount(maxCount)
    , count(initialCount)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        ptrKernel = ptrKernelFactory.getKernel();
    }

    SemResult Semaphore::acquire(std::uint32_t ticksTimeout)
    {
        assert( ptrKernel->isInsideInterrupt() == false );
        if( !ptrKernel->isInsideInterrupt())
        {
            bool waitForever = (ticksTimeout == WAIT_FOREVER);
            std::uint64_t end = ticksTimeout + ptrKernel->getTickCount();
            while (true)
            {
                ptrKernel->enterCritical();
                if (count > 0)
                {
                    --count;
                    ptrKernel->exitCritical();
                    return SemResult::Success;
                }
                else
                {
                    if (ticksTimeout == 0)
                    {
                        ptrKernel->exitCritical();
                        return SemResult::Timeout;
                    }
                    if( !waitForever )
                    {
                        if( ptrKernel->getTickCount() > end)
                        {
                            ptrKernel->exitCritical();
                            return SemResult::Timeout;
                        }
                    }
                    TaskData* ptrTaskData = ptrKernel->getCurrentTask();                                        
                    ptrKernel->resetTaskReady(ptrTaskData, TaskStateType::eBlocked);

                    waitingQueue.enqueue(ptrTaskData);
                    ptrKernel->exitCritical();

                    // Switch context
                    ptrKernel->yield();
                    return SemResult::Success;
                    //
                }
            }
        }
        return SemResult::ErrorCalledFromISR;
    }

    /*Semaphore can be released from interrupt and from any task*/
    void Semaphore::release()
    {
        ptrKernel->enterCritical();
        if (!waitingQueue.isEmpty())
        {
            TaskData* taskToUnblock = waitingQueue.dequeue();
            ptrKernel->setTaskReady(taskToUnblock);
        }
        else if (count < maxCount)
        {
            ++count;
        }

        ptrKernel->exitCritical();

        if (!ptrKernel->isInsideInterrupt())
        {
            ptrKernel->yield();
        }
    }

    void Semaphore::flush()
    {     
        ptrKernel->enterCritical();
        bool isAnyy(false);
        while (!waitingQueue.isEmpty())
        {
            TaskData* taskToUnblock = waitingQueue.dequeue();
            ptrKernel->setTaskReady(taskToUnblock);
            isAnyy = true;
        }
        ptrKernel->exitCritical();
        if( isAnyy && !ptrKernel->isInsideInterrupt() )
        {
            ptrKernel->yield();
        }
    }
}
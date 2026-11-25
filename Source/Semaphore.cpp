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

    SemResult Semaphore::wait(std::uint32_t ticksTimeout)
    {
        assert(ptrKernel != nullptr);
        assert( ptrKernel->isInsideInterrupt() == false );

        if( (ptrKernel == nullptr) || ptrKernel->isInsideInterrupt() )
        {
            return SemResult::ErrorCalledFromISR;
        }

        ptrKernel->enterCritical();
        if (count > 0u)
        {
            --count;
            ptrKernel->exitCritical();
            return SemResult::Success;
        }

        if (ticksTimeout == 0u)
        {
            ptrKernel->exitCritical();
            return SemResult::Timeout;
        }

        // Block until signal() hands over a token or the timeout expires
        ptrKernel->blockCurrentTask(waitingTasks, ticksTimeout);
        ptrKernel->exitCritical();
        ptrKernel->yield();

        // Resumed: the token was handed over directly by signal()/flush(), or the wait timed out
        ptrKernel->enterCritical();
        const WakeReason reason = ptrKernel->getCurrentTask()->getWakeReason();
        ptrKernel->exitCritical();

        return (reason == WakeReason::eSignaled) ? SemResult::Success : SemResult::Timeout;
    }

    /*Semaphore can be released from interrupt and from any task*/
    void Semaphore::signal()
    {
        if (ptrKernel != nullptr)
        {
            bool switchRequired = false;

            ptrKernel->enterCritical();
            if (ptrKernel->wakeOne(waitingTasks, WakeReason::eSignaled, switchRequired) == nullptr)
            {
                // Nobody waiting: keep the token
                if (count < maxCount)
                {
                    ++count;
                }
            }
            ptrKernel->exitCritical();

            if (switchRequired)
            {
                // Pends PendSV; from an ISR the switch happens when the ISR returns
                ptrKernel->yield();
            }
        }
    }

    void Semaphore::flush()
    {
        if (ptrKernel != nullptr)
        {
            ptrKernel->enterCritical();
            const bool switchRequired = ptrKernel->wakeAll(waitingTasks, WakeReason::eSignaled);
            ptrKernel->exitCritical();

            if (switchRequired)
            {
                ptrKernel->yield();
            }
        }
    }
}

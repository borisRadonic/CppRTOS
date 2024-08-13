//
// Created by Boris on 13/08/2024.
//
#include "InternalKernelAcessor.hpp"
#include "KernelFactory.hpp"
#include "Kernel.hpp"


namespace CppRtos
{
    namespace internal
    {
        InternalKernelAcessor::InternalKernelAcessor()
        {
            KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            ptrKernel = ptrKernelFactory.getKernel();
        }

        [[nodiscard]] bool InternalKernelAcessor::isInsideInterrupt() const
        {
            return ptrKernel->isInsideInterrupt();
        }

        void InternalKernelAcessor::yield()
        {
            ptrKernel->yield();
        }

        [[nodiscard]] CppRtos::TaskData* InternalKernelAcessor::getCurrentTask()
        {
            return ptrKernel->getCurrentTask();
        }

        void InternalKernelAcessor::selectHighestPriorityTask()
        {
            ptrKernel->selectHighestPriorityTask();
        }

        void InternalKernelAcessor::tick()
        {
            ptrKernel->tick();
        }

        void InternalKernelAcessor::disableInterrupts()  const
        {
            ptrKernel->disableInterrupts();
        }

        void InternalKernelAcessor::enableInterrupts() const
        {
            ptrKernel->enableInterrupts();
        }

        void InternalKernelAcessor::enterCritical()
        {
            ptrKernel->enterCritical();
        }

        void InternalKernelAcessor::exitCritical()
        {
            ptrKernel->exitCritical();
        }


        [[nodiscard]] std::uint64_t InternalKernelAcessor::getTickCount() const
        {
            return ptrKernel->getTickCount();
        }

        [[nodiscard]] std::uint64_t InternalKernelAcessor::getSysTimerCount() const
        {
            return ptrKernel->getSysTimerCount();
        }
    }
}
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
    enum class SemResult : std::uint8_t
    {
        Success  = 0u,
        ErrorCalledFromISR  = 1u,
        ErrorNotCalledFromISR  = 2u,
        Timeout  = 3u
    };

    class Semaphore
    {
    public:
        
        Semaphore() : _count (0)
        {
             KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            _ptrKernel = ptrKernelFactory.getKernel();
        }

        Semaphore(int initialCount ) : _count(initialCount)
        {
        }

         /*Semaphore can be acquired from any task but not from interrupt!*/
        SemResult acquire(std::uint32_t ticksTimeout)
        {
            assert( _ptrKernel->isInsideInterrupt() == false );
            if( !_ptrKernel->isInsideInterrupt())
            {
                bool waitForever = (ticksTimeout == WAIT_FOREVER);
                std::uint64_t end = ticksTimeout + _ptrKernel->getTickCount();
                while (true)
                {
                    if( !waitForever )
                    {
                        if( _ptrKernel->getTickCount() > end)
                        {
                            return SemResult::Timeout;
                        }
                    }
                    // Spin-wait until the count is greater than zero
                    while (_count.load() == 0)
                    {
                        if( !_blockedTasks.isFull() )
                        {
                            // Disable interrupts
                            _ptrKernel->disableInterrupts();

                            // Get current task
                            TaskData* ptrTaskData = _ptrKernel->getCurrentTask();
                            ptrTaskData->setState( TaskStateType::eBlocked);
                            _blockedTasks.enqueue(ptrTaskData);

                            // Enable interrupts
                            _ptrKernel->enableInterrupts();

                            // Switch context
                            _ptrKernel->switchContext();
                        }
                    }

                    // Attempt to decrement the count
                    int expected = _count.load();
                    if (_count.compare_exchange_weak(expected, expected - 1))
                    {
                        break; // Successfully acquired the semaphore
                    }
                }
                return SemResult::Success;
            }
            return SemResult::ErrorCalledFromISR;
        }

        /*Semaphore can be released from interrupt and from any task*/
        void release()
        {
            // Disable interrupts
            _ptrKernel->disableInterrupts();

            removeNonBlocked();
            _count.fetch_add(1);
            // Check which blocked task has to be set to ready
            if( false == _blockedTasks.isEmpty() )
            {
                TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                ptrUnblockedTask->setState( TaskStateType::eReady );
            }

            // Enable interrupts
            _ptrKernel->enableInterrupts();
        }

        SemResult releaseFromISR()
        {
            assert( _ptrKernel->isInsideInterrupt() );
            if( _ptrKernel->isInsideInterrupt())
            {
                // Disable interrupts
                _ptrKernel->disableInterrupts();

                removeNonBlocked();
                _count.fetch_add(1);
                // Check which blocked task has to be set to ready
                if( false == _blockedTasks.isEmpty() )
                {
                    TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                    ptrUnblockedTask->setState( TaskStateType::eReady );
                }

                // Enable interrupts
                _ptrKernel->enableInterrupts();
                return SemResult::Success;
            }
            return SemResult::ErrorNotCalledFromISR;
        }

         //flush makes all tasks that are currently waiting for the semaphore ready to run,
        // but it does not change the state of the semaphore (it does not increase or decrease the semaphore's count).
        void flush()
        {            
            // Disable interrupts
            _ptrKernel->disableInterrupts();

            /*Remove tasks which are not blocked */
            removeNonBlocked();

            while( false == _blockedTasks.isEmpty() )
            {
                TaskData* ptrUnblockedTask = _blockedTasks.dequeue();
                ptrUnblockedTask->setState( TaskStateType::eReady );
            }
            // Enable interrupts
            _ptrKernel->enableInterrupts();
        }

    private:

        std::atomic<std::int32_t> _count = 0;
        
        Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks  = {};

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
    };
}

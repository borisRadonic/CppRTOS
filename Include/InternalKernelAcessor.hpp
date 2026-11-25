//
// Created by Boris on 13/08/2024.
//

#ifndef INTERNALKERNELUTIL_H
#define INTERNALKERNELUTIL_H
#include "Task.hpp"
#include "WaitList.hpp"

namespace CppRtos
{
    class Kernel;
    namespace internal
    {
        class InternalKernelAcessor final
        {
        public:
            InternalKernelAcessor();

            [[nodiscard]] bool isInsideInterrupt() const;

            void yield();

            [[nodiscard]] CppRtos::TaskData* getCurrentTask();

            void selectHighestPriorityTask();

            void tick();

            void disableInterrupts()  const;

            void enableInterrupts() const;

            void enterCritical();

            void exitCritical();

            [[nodiscard]] std::uint64_t getTickCount() const;

            [[nodiscard]] std::uint64_t getSysTimerCount() const;

            /**
             * @brief Blocks the current task on a wait list (see Kernel::blockCurrentTask). Inside a critical section.
             */
            void blockCurrentTask(WaitList& waitList, std::uint32_t timeoutTicks);

            /**
             * @brief Wakes the highest-priority waiter as signaled (see Kernel::wakeOne). Inside a critical section.
             * @return true if a context switch is required.
             */
            bool wakeOne(WaitList& waitList);

            /**
             * @brief Returns the wake reason of the current task (valid after it resumed from a wait).
             */
            [[nodiscard]] WakeReason getCurrentTaskWakeReason() const;

        private:
            Kernel* ptrKernel = nullptr;
        };
    }
}
#endif //INTERNALKERNELUTIL_H

//
// Created by Boris on 13/08/2024.
//

#ifndef INTERNALKERNELUTIL_H
#define INTERNALKERNELUTIL_H
#include "Task.hpp"

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

        private:
            Kernel* ptrKernel = nullptr;
        };
    }
}
#endif //INTERNALKERNELUTIL_H

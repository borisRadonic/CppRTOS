#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include "Config.hpp"
#include "Fifo.hpp"
#include "Kernel.hpp"

namespace CppRtos
{
    enum class SemResult : std::uint8_t
    {
        Success  = 0u,
        ErrorCalledFromISR  = 1u,
        Timeout  = 2u
    };

    class Semaphore
    {
    public:
        
        explicit Semaphore(std::uint32_t maxCount, std::uint32_t initialCount );
       
        SemResult acquire(std::uint32_t ticksTimeout);
       
        void release();

        //unblock every task pended on a semaphore
        void flush();
       
    private:

       std::uint32_t maxCount;

        std::uint32_t count;
        
        Fifo<TaskData*, Settings::MAX_TASKS> waitingQueue  = {};

        Kernel* ptrKernel = nullptr;

    };
}

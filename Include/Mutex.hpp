#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "Config.hpp"
#include "Kernel.hpp"
#include "Fifo.hpp"


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

        Mutex();

        ~Mutex();
            
        MutexResult acquire(std::uint32_t ticksTimeout);
       
        MutexResult release();

    private:

        bool isAnyTaskBlocking(CppRtos::TaskPriority priority);
       
       private:
            std::int32_t count = 0;
            TaskData* owner = nullptr;
            Fifo<TaskData*, Settings::MAX_TASKS> _blockedTasks = {};
    };
}

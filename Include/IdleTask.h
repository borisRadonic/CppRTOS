#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include "Config.hpp"
#include "Task.hpp"

namespace CppRtos
{

    class IdleTask : public Task<Settings::IDLE_TASK_STACK_SIZE>
    {
    public:

    	IdleTask(): Task<Settings::IDLE_TASK_STACK_SIZE>()
        {        	
        }

        virtual ~IdleTask()
        {
        }

        void run() override
        {
            while(true)
            {                
            }
        }
    };
}

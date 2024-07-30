#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "Task.hpp"

namespace CppRtos
{
    constexpr std::size_t IDLE_TASK_STACK_SIZE = 1024u;

    class IdleTask : public Task
    {
    public:

    	IdleTask(): Task(static_cast<std::uint8_t*>(static_cast<void*>(&stack)), IDLE_TASK_STACK_SIZE)
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

        public: 
            std::aligned_storage_t<IDLE_TASK_STACK_SIZE,4u> stack;
    };
}

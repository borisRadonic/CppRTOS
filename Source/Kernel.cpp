#include "Kernel.hpp"
#include "Task.hpp"

namespace CppRtos
{
    Kernel::Kernel()
    :  state( KernelState::eReset )
    , lastError( KernelError::eOK )			
    , taskCount (0u)
    {
        tasks.fill(nullptr);
        timers.fill(nullptr);

        this->addTask( idleTask );
        currentTask = idleTask.getTaskData();
    }

    void Kernel::updateTimers()
    {
        for (auto timer : timers)
        {
            if (timer != nullptr)
            {
                timer->tick();
            }
        }
    }

    //this function is called only after entering critical section
    void Kernel::setTaskReady( CppRtos::TaskData * ptrTask )
    {
        if( ptrTask != nullptr )
        {
            ptrTask->setState( TaskStateType::eReady );
            std::size_t prio = static_cast<std::size_t>(ptrTask->getPriority());
            assert( prio < readyTasks.size() );
            if( prio < readyTasks.size() )
            {
                readyTasks[prio] = readyTasks[prio] | (1u << ptrTask->getId());
                priorityBitmap |= (1u << prio); // Mark priority as non-empty
            }
        }
    }

    bool Kernel::addTimer(Timer* timer)
    {
        for (auto& t : timers)
        {
            if (t == nullptr)
            {
                t = timer;
                timer->allocate();
                return true;
            }
        }
        return false;
    }

    void Kernel::removeTimer(Timer* timer)
    {
        for (auto& t : timers)
        {
            if (t == timer)
            {
                t->deallocate();
                t = nullptr;
                return;
            }
        }
    }


    void Kernel::start()
    {
        initialize();
        assert( state == KernelState::eReady );
        if( state == KernelState::eReady)
        {
            port.startScheduler();
        }
    }

    void Kernel::addSleepingTask(TaskData* task, std::uint32_t ticks)
    {
        std::uint8_t taskId = task->getId();
        sleepingTasksBitmap |= (1ULL << taskId);
        taskWakeUpTimes[taskId] = getTickCount() + ticks;
        task->setState(TaskStateType::eSleeping);
    }

    void Kernel::selectHighestPriorityTask() 
    {
        port.enterCritical();
        //take next ready task with highest priority
        std::uint8_t startPrio = static_cast<std::uint8_t>(highestTaskPriority);			
        for (std::uint8_t countPrio = startPrio; countPrio < static_cast<std::uint8_t>(TaskPriority::PRIORITY_IDLE); countPrio++ )			
        {
            std::uint32_t taskBitList = readyTasks[countPrio];
            std::uint8_t countTask = 0u;
            while (taskBitList != 0u)
            {
                if(  taskBitList & 1u )
                {
                    TaskData* ptrTaskData = tasks[countTask];						
                    // Preempt the current running task if necessary						
                    if( currentTask->getState() == TaskStateType::eRunning)
                    {
                        setTaskReady( currentTask );
                    }
                    ptrTaskData->setState( TaskStateType::eRunning );
                    currentTask = ptrTaskData;

                    // Clear the bit for this task in readyTasks
                    readyTasks[countPrio] &= ~(1u << countTask);

                    if( readyTasks[countPrio] == 0u )
                    {
                        priorityBitmap &= ~(1u << countPrio);
                    }
                    port.exitCritical();
                    return;						
                }
                taskBitList >>= 1u;
                countTask++;
            }						
        }
        port.exitCritical();
    }

    void Kernel::tick()
    {
        std::uint64_t currentTick = getTickCount();
        // Wake up sleeping tasks whose time has elapsed
        for (std::uint8_t i = 0; i < Settings::MAX_TASKS; ++i)
        {
            if ((sleepingTasksBitmap & (1ULL << i)) && (taskWakeUpTimes[i] <= currentTick))
            {
                TaskData* task = tasks[i];
                setTaskReady(task);
                sleepingTasksBitmap &= ~(1ULL << i);
            }
        }
        updateTimers();
    }
}
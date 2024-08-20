#include "Kernel.hpp"
#include "Task.hpp"

namespace CppRtos
{
    Kernel::Kernel()
    :  state( KernelState::eReset )
    , lastError( KernelError::eOK )			
    , taskCount (0u)
    , port(this)
    {
        tasks.fill(nullptr);
        timers.fill(nullptr);

        this->addTask( idleTask );
        currentTask = idleTask.getTaskData();
    }

    void Kernel::addTask( Task& task  )
    {
        if( taskCount < Settings::MAX_TASKS )
        {
            TaskData* ptrTaskData = task.getTaskData();
            ptrTaskData->setId( taskCount );
            tasks[taskCount] = ptrTaskData;
            taskCount++;

            assert( taskCount <= Settings::MAX_TASKS );
            //add task to the Ready List
            port.enterCritical();

            TaskPriority priority = ptrTaskData->getPriority();

            if( priority > highestTaskPriority )
            {
                highestTaskPriority = priority;
            }

            StackAddr pStack = port.initialiseStack(static_cast<void*>(ptrTaskData->getCurrentStackPtr()), static_cast<void*>(this ));
            ptrTaskData->setCurrentStackPtr( pStack );

            auto prio = static_cast<std::size_t>(priority);
            assert( prio < readyTasks.size() );
            if( prio < readyTasks.size() )
            {
                readyTasks[prio] = readyTasks[prio] | (1u << ptrTaskData->getId());
            }
            port.exitCritical();
        }
    }

    void Kernel::updateTimers() const
    {
        for (auto timer : timers)
        {
            if (timer != nullptr)
            {
                timer->tick();
            }
        }
    }

    void Kernel::incrementCounters() const
    {
        for (auto& c : counters)
        {
            if( c != nullptr )
            {
                c->Increment();
            }
        }
    }

    void Kernel::CheckAlarms() const
    {
        for (auto& a : alarms)
        {
            if( a != nullptr )
            {
                a->CheckAndTrigger();
            }
        }
    }

    //this function is called only after entering critical section
    void Kernel::setTaskReady( CppRtos::TaskData * ptrTask )
    {
        if( ptrTask != nullptr )
        {
            ptrTask->setState( TaskStateType::eReady );
            auto prio = static_cast<std::size_t>(ptrTask->getPriority());
            assert( prio < readyTasks.size() );
            if( prio < readyTasks.size() )
            {
                readyTasks[prio] = readyTasks[prio] | (1u << ptrTask->getId());
                priorityBitmap |= (1u << prio); // Mark priority as non-empty
            }
        }
    }

    void Kernel::resetTaskReady(CppRtos::TaskData* ptrTask, TaskStateType newState)
    {
        if (ptrTask != nullptr)
        {
            // Only reset if the task is currently in the ready state
            if (ptrTask->getState() == TaskStateType::eReady)
            {
                auto prio = static_cast<std::size_t>(ptrTask->getPriority());
                assert(prio < readyTasks.size());
                if (prio < readyTasks.size())
                {
                    readyTasks[prio] &= ~(1u << ptrTask->getId());
                    if (readyTasks[prio] == 0)
                    {
                        priorityBitmap &= ~(1u << prio);
                    }                    
                }
            }
            ptrTask->setState(newState);
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

    bool Kernel::addCounter(Counter* counter)
    {
        if( counter != nullptr )
        {
            for (auto& c : counters)
            {
                if (c == nullptr)
                {
                    c = counter;
                    return true;
                }
            }
        }
        return false;
    }

    void Kernel::removeCounter(Counter* counter)
    {
        if( counter != nullptr )
        {
            for (auto& c : counters)
            {
                if (c == counter)
                {
                    c = nullptr;
                    return;
                }
            }
        }
    }

    bool Kernel::addAlarm(Alarm* alarm)
    {
        if( alarm != nullptr )
        {
            for (auto& a : alarms)
            {
                if (a == nullptr)
                {
                    a = alarm;
                    return true;
                }
            }
        }
        return false;
    }

    void Kernel::removeAlarm(Alarm* alarm)
    {
        if( alarm != nullptr )
        {
            for (auto& a : alarms)
            {
                if (a == alarm)
                {
                    a = nullptr;
                    return;
                }
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
        auto startPrio = static_cast<std::uint8_t>(highestTaskPriority);
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
                    if( currentTask->getState() == TaskStateType::eRunning )
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
        incrementCounters();
        CheckAlarms();
    }
}
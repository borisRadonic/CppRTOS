#include "Kernel.hpp"
#include "Task.hpp"

namespace CppRtos
{
    namespace
    {
        /**
         * @brief Returns the index of the lowest set bit. The value must not be zero.
         */
        inline std::uint32_t lowestSetBit(std::uint32_t value)
        {
            assert(value != 0u);
#if defined(__GNUC__) || defined(__clang__)
            return static_cast<std::uint32_t>(__builtin_ctz(value));
#else
            std::uint32_t index = 0u;
            while ((value & 1u) == 0u)
            {
                value >>= 1u;
                index++;
            }
            return index;
#endif
        }
    }

    Kernel::Kernel()
    :  state( KernelState::eReset )
    , lastError( KernelError::eOK )			
    , taskCount (0u)
    , port(this)
    {
        tasks.fill(nullptr);
        timers.fill(nullptr);
        // Start each round-robin cursor at the last id, so the first selection at a level picks the lowest id.
        lastScheduledId.fill(static_cast<std::uint8_t>(Settings::MAX_TASKS - 1u));

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

            StackAddr pStack = port.initialiseStack(static_cast<void*>(ptrTaskData->getCurrentStackPtr()), static_cast<void*>(this ));
            ptrTaskData->setCurrentStackPtr( pStack );

            // Insert into the ready list and mark the priority level as non-empty
            setTaskReady( ptrTaskData );
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
            // Select the first task to run, so the scheduler starts with the highest-priority ready task.
            port.enterCritical();
            selectHighestPriorityTask();
            port.exitCritical();

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
        // The running task is not kept in the ready lists. Put it back if it was preempted
        // (still running), so it competes with the other ready tasks. Blocked or sleeping
        // tasks have already left the running state and are not re-inserted.
        if( (currentTask != nullptr) && (currentTask->getState() == TaskStateType::eRunning) )
        {
            setTaskReady( currentTask );
        }

        if( priorityBitmap == 0u )
        {
            // Nothing is ready. Cannot happen while the idle task exists; keep the current task.
            return;
        }

        // Lower numeric value means higher priority, so the lowest set bit is the highest ready priority.
        const std::uint32_t prio = lowestSetBit( priorityBitmap );
        const std::uint32_t readyMask = readyTasks[prio];

        // Round-robin within the level: take the first ready task with an id greater than the one
        // scheduled last at this level, otherwise wrap around to the lowest id.
        const std::uint32_t firstCandidate = static_cast<std::uint32_t>(lastScheduledId[prio]) + 1u;
        std::uint32_t candidates = 0u;
        if( firstCandidate < 32u )
        {
            candidates = readyMask & ~((1u << firstCandidate) - 1u);
        }
        if( candidates == 0u )
        {
            candidates = readyMask;
        }

        const std::uint32_t id = lowestSetBit( candidates );
        TaskData* ptrNextTask = tasks[id];
        assert( ptrNextTask != nullptr );

        // Remove the selected task from its ready list
        readyTasks[prio] &= ~(1u << id);
        if( readyTasks[prio] == 0u )
        {
            priorityBitmap &= ~(1u << prio);
        }

        lastScheduledId[prio] = static_cast<std::uint8_t>(id);
        ptrNextTask->setState( TaskStateType::eRunning );
        currentTask = ptrNextTask;
    }

    void Kernel::blockCurrentTask(WaitList& waitList, std::uint32_t timeoutTicks)
    {
        TaskData* ptrTask = currentTask;
        assert( ptrTask != nullptr );
        assert( timeoutTicks != 0u );
        assert( ptrTask->getWaitList() == nullptr );

        const std::uint32_t id = ptrTask->getId();

        // The running task is not in a ready list; if it was made ready meanwhile, resetTaskReady() removes it.
        resetTaskReady( ptrTask, TaskStateType::eBlocked );

        ptrTask->setWakeReason( WakeReason::eNone );
        ptrTask->setWaitList( &waitList );
        ptrTask->setWaitSequence( waitSequenceCounter++ );
        waitList.waitingTasksMask |= (1u << id);

        if( timeoutTicks != WAIT_FOREVER )
        {
            taskWakeUpTimes[id] = getTickCount() + timeoutTicks;
            sleepingTasksBitmap |= (1ULL << id);
        }
    }

    void Kernel::wakeTask(TaskData* ptrTask, WakeReason reason, bool& switchRequired)
    {
        assert( ptrTask != nullptr );
        WaitList* ptrWaitList = ptrTask->getWaitList();
        assert( ptrWaitList != nullptr );

        const std::uint32_t id = ptrTask->getId();
        if( ptrWaitList != nullptr )
        {
            ptrWaitList->waitingTasksMask &= ~(1u << id);
        }
        ptrTask->setWaitList( nullptr );

        // Cancel a pending timeout
        sleepingTasksBitmap &= ~(1ULL << id);

        ptrTask->setWakeReason( reason );
        setTaskReady( ptrTask );

        if( (currentTask == nullptr) || isHigherPriority( ptrTask->getPriority(), currentTask->getPriority() ) )
        {
            switchRequired = true;
        }
    }

    TaskData* Kernel::wakeOne(WaitList& waitList, WakeReason reason, bool& switchRequired)
    {
        std::uint32_t mask = waitList.waitingTasksMask;
        TaskData* ptrBest = nullptr;

        while( mask != 0u )
        {
            const std::uint32_t id = lowestSetBit( mask );
            mask &= ~(1u << id);

            TaskData* ptrCandidate = tasks[id];
            if( ptrBest == nullptr )
            {
                ptrBest = ptrCandidate;
            }
            else if( isHigherPriority( ptrCandidate->getPriority(), ptrBest->getPriority() ) )
            {
                ptrBest = ptrCandidate;
            }
            else if( (ptrCandidate->getPriority() == ptrBest->getPriority()) &&
                     (static_cast<std::int32_t>(ptrCandidate->getWaitSequence() - ptrBest->getWaitSequence()) < 0) )
            {
                // Same priority: the task that started waiting first is woken first (wrap-safe compare)
                ptrBest = ptrCandidate;
            }
        }

        if( ptrBest != nullptr )
        {
            wakeTask( ptrBest, reason, switchRequired );
        }
        return ptrBest;
    }

    bool Kernel::wakeAll(WaitList& waitList, WakeReason reason)
    {
        bool switchRequired = false;
        while( wakeOne( waitList, reason, switchRequired ) != nullptr )
        {
        }
        return switchRequired;
    }

    TaskPriority Kernel::getHighestWaiterPriority(const WaitList& waitList) const
    {
        TaskPriority highest = TaskPriority::PRIORITY_IDLE;
        std::uint32_t mask = waitList.waitingTasksMask;
        while( mask != 0u )
        {
            const std::uint32_t id = lowestSetBit( mask );
            mask &= ~(1u << id);
            const TaskPriority prio = tasks[id]->getPriority();
            if( isHigherPriority( prio, highest ) )
            {
                highest = prio;
            }
        }
        return highest;
    }

    void Kernel::changeTaskPriority(TaskData* ptrTask, TaskPriority newPriority)
    {
        assert( ptrTask != nullptr );
        assert( static_cast<std::size_t>(newPriority) < readyTasks.size() );
        if( (ptrTask == nullptr) || (ptrTask->getPriority() == newPriority) )
        {
            return;
        }

        if( ptrTask->getState() == TaskStateType::eReady )
        {
            // Move the task from the ready list of the old priority to the one of the new priority
            resetTaskReady( ptrTask, TaskStateType::eReady );
            ptrTask->setCurrentPriority( newPriority );
            setTaskReady( ptrTask );
        }
        else
        {
            // Running, blocked or sleeping tasks are not in a ready list
            ptrTask->setCurrentPriority( newPriority );
        }
    }

    void Kernel::tick()
    {
        std::uint64_t currentTick = getTickCount();
        // Wake up sleeping tasks and blocked tasks whose timeout has elapsed
        for (std::uint8_t i = 0; i < Settings::MAX_TASKS; ++i)
        {
            if ((sleepingTasksBitmap & (1ULL << i)) && (taskWakeUpTimes[i] <= currentTick))
            {
                TaskData* task = tasks[i];
                sleepingTasksBitmap &= ~(1ULL << i);
                if (task->getWaitList() != nullptr)
                {
                    // Blocked on a kernel object: the wait timed out
                    bool switchRequired = false; // PendSV is requested by the tick handler anyway
                    wakeTask(task, WakeReason::eTimeout, switchRequired);
                }
                else
                {
                    setTaskReady(task);
                }
            }
        }
        updateTimers();
        incrementCounters();
        CheckAlarms();
    }
}
//
// Created by Boris on 20/08/2024.
//


#include "Alarm.hpp"


namespace CppRtos
{
    Alarm::Alarm(Counter& counter, Callback& cbk, void* arg, std::uint32_t alarmTime, std::uint32_t cycleTime)
    : counter(counter)
    , callback(cbk)
    , callbackArg( arg )
    , alarmTime(alarmTime)
    , cycleTime(cycleTime)
    , active(false)
    {
    }

    Alarm::Alarm(Counter& counter, std::uint32_t alarmTime, std::uint32_t cycleTime, Task* task)
    : counter(counter),
    callback(nullptr),
    callbackArg(nullptr),
    alarmTime(alarmTime),
    cycleTime(cycleTime),
    active(false),
    taskToActivate(task)
    {
    }


    void Alarm::Start()
    {
        startTick = counter.GetCurrentTick();
        active = true;
    }

    void Alarm::CheckAndTrigger()
    {
        if (active)
        {
            std::uint32_t elapsedTime = counter.GetElapsedValue(startTick);
            if (elapsedTime >= alarmTime)
            {
                // Handle cyclic alarms
                if (cycleTime > 0u)
                {
                    startTick = counter.GetCurrentTick();
                    alarmTime = cycleTime;
                }
                else
                {
                    active = false; // Stop one-shot alarm
                }
                if( taskToActivate != nullptr)
                {
                    TaskData* ptrTaskData = taskToActivate->getTaskData();
                    if( ptrTaskData != nullptr )
                    {
                        if( ptrTaskData->getState() != TaskStateType::eRunning)
                        {
                            ptrTaskData->setState(TaskStateType::eReady);
                            taskToActivate->yield();
                        }
                    }
                }
                else
                {
                    callback( callbackArg );
                }
            }
        }
    }
}

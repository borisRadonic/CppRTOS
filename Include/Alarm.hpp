//
// Created by Boris on 20/08/2024.
//

#ifndef ALARM_HPP
#define ALARM_HPP

#include <cstdint>
#include <functional>
#include "Counter.hpp"
#include "Task.hpp"

namespace CppRtos
{
    /**
    * @brief The Alarm class is responsible for triggering an action (callback or task activation)
    *        at a specified time or at regular intervals (cyclic).
    */
    class Alarm
    {
    public:
        /**
         * @brief Deleted copy constructor to prevent copying of Alarm objects.
         */
        Alarm(const Alarm&) = delete;

        /**
         * @brief Deleted copy assignment operator to prevent copying of Alarm objects.
         */
        Alarm& operator=(const Alarm&) = delete;

        /**
         * @brief Deleted move constructor to prevent moving of Alarm objects.
         */
        Alarm(Alarm&&) = delete;

        /**
         * @brief Deleted move assignment operator to prevent moving of Alarm objects.
         */
        Alarm& operator=(Alarm&&) = delete;

        /**
         * @brief Defines a callback function type that can be used with the alarm.
         *
         * The callback function takes a single argument, which is a pointer to any type of data.
         */
        using Callback = std::function<void(void*)>;

        /**
         * @brief Constructs an Alarm object that triggers a callback function.
         *
         * @param counter Reference to the counter that the alarm will use for timing.
         * @param cbk The callback function to be executed when the alarm triggers.
         * @param arg The argument to be passed to the callback function.
         * @param alarmTime The time at which the alarm should trigger, relative to when it was started.
         * @param cycleTime The interval for cyclic alarms. If set to 0, the alarm is a one-shot alarm.
         */
        Alarm(Counter& counter, Callback& cbk, void* arg, std::uint32_t alarmTime, std::uint32_t cycleTime);

        /**
         * @brief Constructs an Alarm object that activates a task when triggered.
         *
         * @param counter Reference to the counter that the alarm will use for timing.
         * @param alarmTime The time at which the alarm should trigger, relative to when it was started.
         * @param cycleTime The interval for cyclic alarms. If set to 0, the alarm is a one-shot alarm.
         * @param task Pointer to the task that should be activated when the alarm triggers.
         */
        Alarm(Counter& counter, std::uint32_t alarmTime, std::uint32_t cycleTime, Task* task);

        /**
         * @brief Starts the alarm.
         *
         * This method records the current tick of the counter and sets the alarm to active.
         */
        void Start();

        /**
         * @brief Stops the alarm.
         *
         * This method deactivates the alarm, preventing it from triggering.
         */
        void Stop()
        {
            active = false;
        }

        /**
         * @brief Checks if the alarm should trigger based on the current counter value.
         *
         * This method is typically called periodically to determine if the alarm's time has been reached.
         * If the alarm has expired, it will either trigger the associated callback or activate the task.
         */
        void CheckAndTrigger();

    private:
        Callback callback;                 /**< The callback function to be executed when the alarm triggers. */
        void* callbackArg = nullptr;       /**< The argument to be passed to the callback function. */
        Counter& counter;                  /**< Reference to the counter used for timing the alarm. */
        std::uint32_t alarmTime = 0u;      /**< The time at which the alarm should trigger, relative to when it was started. */
        std::uint32_t cycleTime = 0u;      /**< The interval for cyclic alarms. 0 for one-shot alarms. */
        std::uint32_t startTick = 0u;      /**< The tick value when the alarm was started. Used to calculate the trigger time. */
        bool active = false;               /**< Indicates whether the alarm is currently active. */
        Task* taskToActivate = nullptr;    /**< Pointer to the task that should be activated when the alarm triggers. */
    };
}

#endif //ALARM_HPP

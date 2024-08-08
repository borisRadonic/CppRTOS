#pragma once

#include <cstdint>
#include <functional>

namespace CppRtos
{
    /**
     * @brief Timer class for scheduling and executing periodic tasks in a Real-Time Operating System (RTOS).
     * 
     * The Timer class provides a mechanism to execute a callback function periodically or once after a specified
     * number of ticks. It supports auto-reloading to repeatedly execute the callback at the specified interval.
     */
    class Timer
    {
    public:
        using Callback = std::function<void(void*)>;

        /**
         * @brief Constructs a Timer with a specified callback, period, and auto-reload option.
         * 
         * @param cbck The callback function to be executed when the timer expires.
         * @param periodTicks The period of the timer in ticks.
         * @param autoReload If true, the timer will automatically reload and continue running after each expiration.
         * @param arg The argument to pass to the callback function.
         */
        Timer(Callback cbck, std::uint32_t periodTicks, bool autoReload, void* arg);
      
        /**
         * @brief Starts the timer.
         * 
         * The timer begins counting down from the specified period. If auto-reload is enabled,
         * the timer will reset and continue running after each expiration.
         */
        void start();

        /**
         * @brief Stops the timer.
         * 
         * The timer stops running and will not execute the callback function.
         */
        void stop();

        /**
         * @brief Resets the timer.
         * 
         * The timer resets its countdown to the specified period but does not start it.
         */
        void reset();

        /**
         * @brief Sets the period of the timer.
         * 
         * @param periodTicks The new period of the timer in ticks.
         */
        void setPeriod(std::uint32_t periodTicks);

        /**
         * @brief Checks if the timer is currently running.
         * 
         * @return true if the timer is running, false otherwise.
         */
        [[nodiscard]] bool isRunning() const;

        /**
         * @brief Advances the timer by one tick.
         * 
         * If the timer is running, this method decrements the remaining ticks. If the remaining ticks reach zero,
         * the callback function is executed, and the timer is either stopped or reloaded depending on the auto-reload setting.
         */
        void tick();

        /**
         * @brief Checks if the timer is allocated.
         * 
         * @return true if the timer is allocated, false otherwise.
         */
        [[nodiscard]] bool isAllocated() const;

        /**
         * @brief Allocates the timer.
         * 
         * Marks the timer as allocated.
         */
        void allocate();
        
        /**
         * @brief Deallocates the timer.
         * 
         * Marks the timer as deallocated and stops it from running.
         */
        void deallocate();

    private:
        Callback callback;                  /**< The callback function to be executed when the timer expires. */
        std::uint32_t periodTicks = 0u;     /**< The period of the timer in ticks. */
        std::uint32_t remainingTicks = 0u;  /**< The remaining ticks before the timer expires. */
        bool autoReload = false;            /**< Whether the timer should automatically reload after expiration. */
        bool running = false;               /**< Whether the timer is currently running. */
        bool allocated = false;             /**< Whether the timer is allocated. */
        void* arg;                          /**< Argument to pass to the callback function. */
    };
}
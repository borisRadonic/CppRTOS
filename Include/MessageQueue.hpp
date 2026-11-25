#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include "InternalKernelAcessor.hpp"
#include "Config.hpp"
#include "Fifo.hpp"
#include "WaitList.hpp"
#include "Kernel.hpp"

namespace CppRtos
{
    enum class MsgQueueResult : std::uint8_t
    {
        Success = 0u,
        ErrorQueueFull = 1u,
        ErrorQueueEmpty = 2u,
        ErrorCalledFromISR = 3u,
        Timeout = 4u
    };
   
 /**
     * @brief MessageQueue class for intertask communication in a Real-Time Operating System (RTOS).
     * 
     * The MessageQueue class provides a mechanism for tasks and interrupt service routines (ISRs) to send and receive messages.
     * Messages are queued in a first-in-first-out (FIFO) order. Multiple tasks can send to and receive from the same message queue.
     */
    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    class MessageQueue
    {
    public:
        /**
         * @brief Constructs a MessageQueue.
         */
        MessageQueue();

        /**
         * @brief Sends a message to the message queue.
         * 
         * @param message The message to send.
         * @param timeout The number of ticks to wait for free space if the message queue is full.
         * @return MsgQueueResult The result of the send operation.
         * - MsgQueueResult::Success: The message was successfully sent.
         * - MsgQueueResult::ErrorQueueFull: The message queue is full and no space is available.
         * - From an ISR the call never blocks: if the queue is full, MsgQueueResult::ErrorQueueFull is returned.
         * - MsgQueueResult::Timeout: The send operation timed out before space became available.
         */
        MsgQueueResult send(const T_MESSAGE& message, std::uint32_t timeout);

        /**
         * @brief Receives a message from the message queue.
         * 
         * @param message The buffer to copy the received message into.
         * @param timeout The number of ticks to wait for a message if the queue is empty.
         * @return MsgQueueResult The result of the receive operation.
         * - MsgQueueResult::Success: The message was successfully received.
         * - MsgQueueResult::ErrorQueueEmpty: The message queue is empty and no message is available.
         * - MsgQueueResult::ErrorCalledFromISR: The receive operation was called from an ISR, which is not allowed.
         * - MsgQueueResult::Timeout: The receive operation timed out before a message became available.
         */
        MsgQueueResult receive(T_MESSAGE& message, std::uint32_t timeout);

        /**
         * @brief Gets the number of messages currently queued.
         * 
         * @return std::size_t The number of messages currently queued.
         */
        std::size_t getNumMsg() const;

    private:
        Fifo<T_MESSAGE, MAX_MESSAGES> queue;    /**< Fifo for storing messages */
        WaitList waitingSenders;                /**< Tasks waiting for free space */
        WaitList waitingReceivers;              /**< Tasks waiting for a message */
    };

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MessageQueue<T_MESSAGE, MAX_MESSAGES>::MessageQueue() = default;

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MsgQueueResult MessageQueue<T_MESSAGE, MAX_MESSAGES>::send(const T_MESSAGE& message, std::uint32_t timeout)
    {
        internal::InternalKernelAcessor kernelAccessor;
        const bool fromIsr = kernelAccessor.isInsideInterrupt();
        const bool waitForever = (timeout == WAIT_FOREVER);

        kernelAccessor.enterCritical();
        const std::uint64_t deadline = kernelAccessor.getTickCount() + timeout;

        while (queue.isFull())
        {
            // An ISR never blocks; timeout 0 means no wait
            if (fromIsr || (timeout == 0u))
            {
                kernelAccessor.exitCritical();
                return MsgQueueResult::ErrorQueueFull;
            }

            std::uint32_t remaining = WAIT_FOREVER;
            if (!waitForever)
            {
                const std::uint64_t now = kernelAccessor.getTickCount();
                if (now >= deadline)
                {
                    kernelAccessor.exitCritical();
                    return MsgQueueResult::Timeout;
                }
                remaining = static_cast<std::uint32_t>(deadline - now);
            }

            // Block until a receiver frees a slot or the timeout expires
            kernelAccessor.blockCurrentTask(waitingSenders, remaining);
            kernelAccessor.exitCritical();
            kernelAccessor.yield();
            kernelAccessor.enterCritical();

            if (kernelAccessor.getCurrentTaskWakeReason() == WakeReason::eTimeout)
            {
                kernelAccessor.exitCritical();
                return MsgQueueResult::Timeout;
            }
            // Signaled: re-check, another sender may have taken the free slot meanwhile
        }

        queue.enqueue(message);
        const bool switchRequired = kernelAccessor.wakeOne(waitingReceivers);
        kernelAccessor.exitCritical();

        if (switchRequired)
        {
            // Pends PendSV; from an ISR the switch happens when the ISR returns
            kernelAccessor.yield();
        }
        return MsgQueueResult::Success;
    }

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MsgQueueResult MessageQueue<T_MESSAGE, MAX_MESSAGES>::receive(T_MESSAGE& message, std::uint32_t timeout)
    {
        internal::InternalKernelAcessor kernelAccessor;
        if (kernelAccessor.isInsideInterrupt())
        {
            return MsgQueueResult::ErrorCalledFromISR;
        }
        const bool waitForever = (timeout == WAIT_FOREVER);

        kernelAccessor.enterCritical();
        const std::uint64_t deadline = kernelAccessor.getTickCount() + timeout;

        while (queue.isEmpty())
        {
            if (timeout == 0u)
            {
                kernelAccessor.exitCritical();
                return MsgQueueResult::ErrorQueueEmpty;
            }

            std::uint32_t remaining = WAIT_FOREVER;
            if (!waitForever)
            {
                const std::uint64_t now = kernelAccessor.getTickCount();
                if (now >= deadline)
                {
                    kernelAccessor.exitCritical();
                    return MsgQueueResult::Timeout;
                }
                remaining = static_cast<std::uint32_t>(deadline - now);
            }

            // Block until a sender posts a message or the timeout expires
            kernelAccessor.blockCurrentTask(waitingReceivers, remaining);
            kernelAccessor.exitCritical();
            kernelAccessor.yield();
            kernelAccessor.enterCritical();

            if (kernelAccessor.getCurrentTaskWakeReason() == WakeReason::eTimeout)
            {
                kernelAccessor.exitCritical();
                return MsgQueueResult::Timeout;
            }
            // Signaled: re-check, another receiver may have taken the message meanwhile
        }

        message = queue.dequeue();
        const bool switchRequired = kernelAccessor.wakeOne(waitingSenders);
        kernelAccessor.exitCritical();

        if (switchRequired)
        {
            kernelAccessor.yield();
        }
        return MsgQueueResult::Success;
    }

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    std::size_t MessageQueue<T_MESSAGE, MAX_MESSAGES>::getNumMsg() const
    {
        internal::InternalKernelAcessor kernelAccessor;
        kernelAccessor.enterCritical();
        std::size_t numMsg = queue.getSize();
        kernelAccessor.exitCritical();
        return numMsg;
    }
}

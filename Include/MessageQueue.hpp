#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <algorithm>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Mutex.hpp"
#include "Config.hpp"

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
         * - MsgQueueResult::ErrorCalledFromISR: The send operation was called from an ISR with a timeout other than NoWait.
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
        Fifo<T_MESSAGE, MAX_MESSAGES> queue; /**< Fifo for storing messages */
        mutable Mutex mtx; /**< Mutex to protect access to the queue */
    };

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MessageQueue<T_MESSAGE, MAX_MESSAGES>::MessageQueue()
    {       
    }

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MsgQueueResult MessageQueue<T_MESSAGE, MAX_MESSAGES>::send(const T_MESSAGE& message, std::uint32_t timeout)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();

        if (ptrKernel->isInsideInterrupt() && (timeout != 0u) )
        {
            return MsgQueueResult::ErrorCalledFromISR;
        }

        if (mtx.acquire(timeout) != MutexResult::Success)
        {
            return MsgQueueResult::Timeout;
        }

        if (queue.isFull())
        {
            if (timeout == 0u)
            {
               mtx.release(); 
               return MsgQueueResult::ErrorQueueFull;
            }
            else if (timeout == WAIT_FOREVER)
            {
                // Wait indefinitely until space is available
                while (queue.isFull())
                {
                    mtx.release();
                    ptrKernel->yield();
                    if (mtx.acquire(timeout) != MutexResult::Success)
                    {
                        return MsgQueueResult::Timeout;
                    }
                }
            }
            else
            {
                // Wait for the specified timeout
                std::uint64_t end = ptrKernel->getTickCount() + timeout;
                while (queue.isFull())
                {
                    if (ptrKernel->getTickCount() > end)
                    {
                        mtx.release();
                        return MsgQueueResult::Timeout;
                    }
                    mtx.release();
                    ptrKernel->yield();
                    if (mtx.acquire(timeout) != MutexResult::Success)
                    {
                        return MsgQueueResult::Timeout;
                    }
                }
            }
        }
        // Queue the message
        queue.enqueue(message);
        mtx.release();
        return MsgQueueResult::Success;
    }

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    MsgQueueResult MessageQueue<T_MESSAGE, MAX_MESSAGES>::receive(T_MESSAGE& message, std::uint32_t timeout)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();

        if (ptrKernel->isInsideInterrupt())
        {
            return MsgQueueResult::ErrorCalledFromISR;
        }

        if (mtx.acquire(timeout) != MutexResult::Success)
        {
            return MsgQueueResult::Timeout;
        }
       
        if (queue.isEmpty())
        {
            if (timeout == 0u)
            {
                mtx.release();
                return MsgQueueResult::ErrorQueueEmpty;
            }
            else if (timeout == WAIT_FOREVER)
            {
                // Wait indefinitely until a message is available
                while (queue.isEmpty())
                {
                    mtx.release();
                    ptrKernel->yield();

                    /*todo:test*/
                    if(mtx.acquire(timeout) != MutexResult::Success)
                    {
                        return MsgQueueResult::Timeout;
                    }
                }
            }
            else
            {
                // Wait for the specified timeout
                std::uint64_t end = ptrKernel->getTickCount() + timeout;
                while (queue.isEmpty())
                {
                    if (ptrKernel->getTickCount() > end)
                    {
                         mtx.release();
                        return MsgQueueResult::Timeout;
                    }
                    mtx.release();
                    ptrKernel->yield();
                    if (mtx.acquire(timeout) != MutexResult::Success)
                    {
                        return MsgQueueResult::Timeout;
                    }
                }                
            }
        }
         // Dequeue the message
        message = queue.dequeue();
        mtx.release();
        return MsgQueueResult::Success;   
    }

    template<typename T_MESSAGE, std::size_t MAX_MESSAGES>
    std::size_t MessageQueue<T_MESSAGE, MAX_MESSAGES>::getNumMsg() const
    {        
        if (mtx.acquire(WAIT_FOREVER) != MutexResult::Success)
        {
            return 0u;
        }
        std::size_t numMsg = queue.getSize();
        mtx.release();
        return numMsg;
    }
}

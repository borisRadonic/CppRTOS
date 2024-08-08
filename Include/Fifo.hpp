#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>
#include <atomic>

#include "Config.hpp"

namespace CppRtos
{
    /**
    * @brief A template class that implements a fixed-size First-In-First-Out (FIFO) buffer.
    *
    * The Fifo class provides basic queue operations such as enqueue, dequeue,
    * and random access to elements. It is implemented as a circular buffer.
    *
    * @tparam T The type of elements stored in the FIFO buffer.
    * @tparam MAX_ITEMS The maximum number of items the FIFO can hold.
    */
    template<typename T, std::size_t MAX_ITEMS>
    class Fifo
    {
    public:
        /**
         * @brief Constructs an empty FIFO buffer.
         */
        Fifo() : head(0), tail(0), size(0)
        {
        }

        /**
         * @brief Adds an item to the FIFO buffer.
         *
         * If the buffer is full, the item is not added.
         *
         * @param item The item to enqueue.
         */
        inline void enqueue(const T& item)
        {
            if (size < MAX_ITEMS)
            {
                buffer[tail] = item;
                tail = (tail + 1) % MAX_ITEMS;
                size++;
            }
        }

        /**
         * @brief Removes and returns the item at the front of the FIFO buffer.
         *
         * If the buffer is empty, returns a default-constructed T object.
         *
         * @return The item at the front of the buffer.
         */
        inline T dequeue()
        {
            if (size > 0)
            {
                T item = buffer[head];
                head = (head + 1) % MAX_ITEMS;
                size--;
                return item;
            }
            return T(); // Return default-constructed object if empty
        }

        /**
         * @brief Retrieves the item at the specified index.
         *
         * If the index is out of bounds, returns a default-constructed T object.
         *
         * @param index The index of the item to retrieve.
         * @return The item at the specified index.
         */
        inline T getAt(const std::size_t index)
        {
            if(index < MAX_ITEMS)
            {
                return buffer[index];
            }
            return T(); // Return default-constructed object if index is out of bounds
        }

        /**
         * @brief Removes the item at the specified position.
         *
         * The function shifts all subsequent items to fill the gap.
         *
         * @param pos The position of the item to remove.
         */
        void removeAt(std::size_t pos)
        {
            for (std::size_t i = pos; i != (tail - 1 + MAX_ITEMS) % MAX_ITEMS; i = (i + 1) % MAX_ITEMS)
            {
                std::size_t next = (i + 1) % MAX_ITEMS;
                buffer[i] = buffer[next];
            }
            tail = (tail - 1 + MAX_ITEMS) % MAX_ITEMS;
            size--;
        }

        /**
         * @brief Checks if the FIFO buffer is empty.
         *
         * @return true if the buffer is empty, false otherwise.
         */
        [[nodiscard]] inline bool isEmpty() const
        {
            return (size == 0u);
        }

        /**
         * @brief Checks if the FIFO buffer is full.
         *
         * @return true if the buffer is full, false otherwise.
         */
        [[nodiscard]] inline bool isFull() const
        {
            return (size == MAX_ITEMS);
        }

        /**
         * @brief Gets the current number of items in the FIFO buffer.
         *
         * @return The number of items in the buffer.
         */
        [[nodiscard]] std::size_t getSize() const
        {
            return size;
        }

    private:
        std::array<T, MAX_ITEMS> buffer = {}; ///< The buffer that stores the items.
        std::size_t head = 0u;                ///< The index of the first item in the buffer.
        std::size_t tail = 0u;                ///< The index of the next position to enqueue an item.
        std::size_t size = 0u;                ///< The current number of items in the buffer.
    };
}

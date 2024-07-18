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
    template<typename T, std::size_t MAX_ITEMS>
    class Fifo
    {
    public:
        Fifo() : _head(0), _tail(0), _size(0)
        {
        }

        inline void enqueue(const T& item)
        {
            if (_size < MAX_ITEMS)
            {
        	    _buffer[_tail] = item;
	            _tail = (_tail + 1) % MAX_ITEMS;
	            _size++;
	        }
        }

        inline T dequeue()
        {
            if (_size > 0)
 	        {
        	    T item = _buffer[_head];
	            _head = (_head + 1) % MAX_ITEMS;
	            _size--;
        	    return item;
            }
		    return nullptr;
        }

        inline T getAt(const std::size_t index)
        {
            if( index < MAX_ITEMS )
            {
                return _buffer[index];
            }
            return nullptr;
        }

        void removeAt(std::size_t pos)
        {
            for (std::size_t i = pos; i != (_tail - 1 + MAX_ITEMS) % MAX_ITEMS; i = (i + 1) % MAX_ITEMS)
            {
                std::size_t next = (i + 1) % MAX_ITEMS;
                _buffer[i] = _buffer[next];
            }
            _tail = (_tail - 1 + MAX_ITEMS) % MAX_ITEMS;
            _size--;
        }

        inline bool isEmpty() const
        {
            return (_size == 0u);
        }

        inline bool isFull() const
        {
            return (_size == MAX_ITEMS);
        }

        std::size_t getSize() const
        {
            return _size;
        }
    private:
        std::array<T, MAX_ITEMS> _buffer = {};
        std::size_t _head = 0u;
        std::size_t _tail = 0u;
        std::size_t _size = 0u;
    };
}

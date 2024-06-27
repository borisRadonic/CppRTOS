#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string_view>
#include <algorithm>
#include <atomic>
#include "Kernel.hpp"
#include "Config.hpp"

namespace CppRtos
{   
    template<typename T, std::size_t MAX_ITEMS>
    class MessageQueue
    {
    public:
        MessageQueue() : _head(0), _tail(0), _size(0)
        {
            KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
            _ptrKernel = ptrKernelFactory.getKernel();
        }

        // Adds an item to the queue
        void send(const T& item)
        {
            assert( _ptrKernel->isInsideInterrupt() == false );
            if( !_ptrKernel->isInsideInterrupt())
            {
                while (true)
                {
                    // Wait if the queue is full
                    if (_size.load() < MAX_ITEMS)
                    {
                        // Disable interrupts
                        _ptrKernel->disableInterrupts();
                        if (_size.load() < MAX_ITEMS)
                        {
                            _buffer[tail] = item;
                            _tail = (tail + 1) % MAX_ITEMS;
                            _size.fetch_add(1);
                            
                            // Enable interrupts
                            _ptrKernel->enableInterrupts();

                            break;
                        }
                        // Enable interrupts
                        _ptrKernel->enableInterrupts();
                    }
                    // Optionally yield to other tasks
                    _ptrKernel->yield();
                }
            }
        }
        bool sendFromISR(const T& item)
        {
            assert( _ptrKernel->isInsideInterrupt() );
            if( _ptrKernel->isInsideInterrupt())
            {
                if (_size.load() < MAX_ITEMS)
                {
                    // Disable interrupts
                    _ptrKernel->disableInterrupts();

                    _buffer[tail] = item;
                    _tail = (tail + 1) % MAX_ITEMS;
                    _size.fetch_add(1);

                    // Enable interrupts
                    _ptrKernel->enableInterrupts();
                    return true;
                }
            }
            return false;
        }

        // Removes an item from the queue
        T receive( int timeout /*todo:timeout*/)
        {
            assert( _ptrKernel->isInsideInterrupt() == false );
            if( !_ptrKernel->isInsideInterrupt())
            {
                while (true)
                {
                    // Wait if the queue is empty
                    if (_size.load() > 0u)
                    {
                        // Disable interrupts
                        _ptrKernel->disableInterrupts();

                        if (_size.load() > 0u)
                        {
                            T item = _buffer[_head];
                            _head = (_head + 1u) % MAX_ITEMS;
                            _size.fetch_sub(1u);
                            
                            // Enable interrupts
                            _ptrKernel->enableInterrupts();

                            return item;
                        }
                        // Enable interrupts
                        _ptrKernel->enableInterrupts();
                    }
                    // Optionally yield to other tasks  ///todo: ?????
                    _ptrKernel->yield();
                }
            }
            return nullptr;
        }

        // Checks if the queue is empty
        bool isEmpty() const
        {
            return ( _size.load() == 0u );
        }

        // Checks if the queue is full
        bool isFull() const
        {
            return ( _size.load() == MAX_ITEMS );
        }

        // Returns the current size of the queue
        std::size_t getSize() const
        {
            return _size.load();
        }

    private:
        std::array<T, MAX_ITEMS> _buffer = {};
        std::atomic<std::size_t> _head;
        std::atomic<std::size_t> _tail;
        std::atomic<std::size_t> _size;

        Kernel* _ptrKernel = nullptr;
    };
}

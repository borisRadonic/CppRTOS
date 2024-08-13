//
// Created by Boris on 13/08/2024.
//

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

namespace CppRtos
{
    class SpinLock final
    {
    public:
        SpinLock() : flag(false)
        {
        }

        void lock()
        {
            // Continuously try to set the flag to true until it succeeds
            while (flag.exchange(true, std::memory_order_acquire))
            {
                // Busy-wait (spin) until the lock is acquired
            }
        }

        void unlock()
        {
            // Release the lock by setting the flag to false
            flag.store(false, std::memory_order_release);
        }

    private:
        std::atomic<bool> flag; // Atomic flag indicating lock status
    };
}

#endif //SPINLOCK_H

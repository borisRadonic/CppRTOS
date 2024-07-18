#pragma once


#include <cstdint>
#include <functional>
#include <utility>

namespace CppRtos
{
    class Kernel; // Forward declaration

    class Timer
    {
    public:
        using Callback = std::function<void(void*)>;

        Timer(Callback cbck, std::uint32_t periodTicks, bool autoReload , void* arg);
      
        void start();

        void stop();

        void reset();

        void setPeriod(std::uint32_t periodTicks);

        bool isRunning() const;

        void tick();

        bool isAllocated() const;

        void allocate();
        
        void deallocate();

    private:
        Callback callback;
        std::uint32_t periodTicks = 0u;
        std::uint32_t remainingTicks = 0u;
        bool autoReload = false;
        bool running = false;
        bool allocated = false;
        void* arg; // Argument to pass to the callback
    };
}

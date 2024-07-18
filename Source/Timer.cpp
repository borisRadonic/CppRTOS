#include "Timer.hpp"

namespace CppRtos
{

    Timer::Timer(Callback cbck, std::uint32_t periodTicks, bool autoReload, void* arg)
          : callback(cbck)
          , periodTicks(periodTicks)
          , remainingTicks(periodTicks)
          , autoReload(autoReload)
          , running(false)
          , allocated(false)
          , arg(arg)
        {
        }


    void Timer::start()
    {
        running = true;
        remainingTicks = periodTicks;
    }

    void Timer::stop()
    {
        running = false;
    }

    void Timer::reset()
    {
        remainingTicks = periodTicks;
    }

    void Timer::setPeriod(std::uint32_t periodTicks)
    {
        this->periodTicks = periodTicks;
        if (running)
        {
            remainingTicks = periodTicks;
        }
    }

    bool Timer::isRunning() const
    {
        return running;
    }

    void Timer::tick()
    {
        if (running && ( --remainingTicks == 0 ))
        {
            if (callback)
            {
                callback(arg);
            }

            if (autoReload)
            {
                remainingTicks = periodTicks;
            }
            else
            {
                running = false;
            }           
        }       
    }

    bool Timer::isAllocated() const
    {
        return allocated;
    }

    void Timer::allocate()
    {
        allocated = true;
    }

    void Timer::deallocate()
    {
        allocated = false;
        running = false;
    }
}
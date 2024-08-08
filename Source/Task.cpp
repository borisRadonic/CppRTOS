#include "Task.hpp"
#include "KernelFactory.hpp"
#include "Kernel.hpp"

namespace CppRtos
{
    Task::Task( std::uint8_t* stack, std::size_t size )
    :ptrStack(stack)
    {			
        std::size_t start = reinterpret_cast<std::size_t>(stack);
        data.setStartStack(stack);
        data.setTopStack(reinterpret_cast<void*>(start + size));
        data.setTaskInterfacePtr(static_cast<ITask*>(this));
    }

    Task::Task():ptrStack(nullptr)
    {		
    }

    Task::~Task()
    {
    }

    void Task::setStack( std::uint8_t* stack, std::size_t size )
    {
        std::size_t start = reinterpret_cast<std::size_t>(stack);
        data.setStartStack(stack);
        data.setTopStack(reinterpret_cast<void*>(start + size));
        data.setTaskInterfacePtr(static_cast<ITask*>(this));
    }

    void Task::sleep(std::uint32_t ticks)
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();
        assert(ptrKernel->isInsideInterrupt() == false);

        ptrKernel->enterCritical();
        // Set the wake-up time and put the task to sleep
        ptrKernel->addSleepingTask(this->getTaskData(), ticks);

        ptrKernel->exitCritical();

        // Yield to switch context
        ptrKernel->yield();
    }

    void Task::yield()
    {
        KernelFactory& ptrKernelFactory = KernelFactory::getInstance();
        Kernel* ptrKernel = ptrKernelFactory.getKernel();
        assert(ptrKernel->isInsideInterrupt() == false);
        if (!ptrKernel->isInsideInterrupt())
        {
            ptrKernel->yield();
        }
    }

}
#include "Port.hpp"
#include "Kernel.hpp"


namespace CppRtos
{
	namespace Port
	{
		CppRtos::TaskData* Port::getCurrentTask()
		{
			return this->ptrKernel->getCurrentTask();
		}

		void Port::selectHighestPriorityTask()
		{
			this->ptrKernel->selectHighestPriorityTask();
		}

		void Port::tick()
		{
			this->ptrKernel->tick();
		}

		TaskData* Port::getCurrentTask() const
		{
			return this->ptrKernel->getCurrentTask();
		}

		void Port::setTaskReady(CppRtos::TaskData* ptrTask)
		{
			return this->ptrKernel->setTaskReady(ptrTask);
		}

		void Port::setCurrentTask(TaskData* task)
		{
			return this->ptrKernel->setCurrentTask(task);
		}

		void Port::resetTaskReady(CppRtos::TaskData* ptrTask, TaskStateType newState)
		{
			return this->ptrKernel->resetTaskReady(ptrTask, newState);
		}
	}
}
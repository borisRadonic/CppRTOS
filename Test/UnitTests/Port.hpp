#pragma once
#include <assert.h>
#include "Interface.hpp"
#include <exception>
#include <string>
#include "Task.hpp"

/*Only for Unit Tests*/

namespace CppRtos
{
	class Kernel;

	class UnitTestException : public std::exception
	{
	public:
		explicit UnitTestException(const std::string& message) : message_(message) {}
		
		virtual const char* what() const noexcept override
		{
			return message_.c_str();
		}
	private:
		std::string message_;
	};

	namespace Port
	{
		class Port : public IPort
		{
		public:

			explicit Port(Kernel* kernel)
				:tickCount(0u)
				,sysTimerCount(0u),
				ptrKernel(kernel)
			{
			}

			~Port()
			{
			}


			CppRtos::TaskData* getCurrentTask();

			void selectHighestPriorityTask();

			void tick();


			inline void unitTestSetisInsideInterrupt(bool value)
			{
				insideInterrupt = value;
			}

			inline bool isInsideInterrupt(void) const override
			{
				return insideInterrupt;
			}

			inline void yield()
			{
				//this is trick to come back in Test (used for Mutex, Semaphore and Queue tests)
				throw UnitTestException("yield");
			}

			TaskData* getCurrentTask() const;

			void setTaskReady(CppRtos::TaskData* ptrTask);

			void setCurrentTask(TaskData* task);

			void resetTaskReady(CppRtos::TaskData* ptrTask, TaskStateType newState);
			

			void disableInterrupts(void)  const override
			{
			}

			void enableInterrupts(void) const override
			{
			}

			void enterCritical(void)  override
			{

			}

			void exitCritical(void) override
			{
			}

			inline void incrementTickCount()
			{
				tickCount++;
			}

			inline void incrementSysTimerCount()
			{
				sysTimerCount++;
			}

			inline std::uint64_t getTickCount() const
			{
				return tickCount;
			}

			inline std::uint64_t getSysTimerCount() const
			{
				return sysTimerCount;
			}

			inline void memoryBarrier(void) const
			{

			}

			void validateInterruptPriority(void) const override
			{
			}

			void startScheduler(void) override
			{
			}

			void endScheduler(void) const override
			{
			}

			void startFirstTask(void) const
			{
			}

			void taskExitError(void) override
			{
			}

			void* initialiseStack(void* pxTopOfStack, void* pvParameters) override
			{
				return nullptr;
			}

		private:


			std::uint64_t 	tickCount = 0u;

			std::uint64_t 	sysTimerCount = 0u;

			bool insideInterrupt = false;

			Kernel* ptrKernel = nullptr;



		};
	}
}

#pragma once
#include <assert.h>
#include "Interface.hpp"
#include <exception>
#include <string>

/*Only for Unit Tests*/

namespace CppRtos
{

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

			Port()
				:_tickCount(0u)
				, _sysTimerCount(0u)
			{
			}

			~Port()
			{
			}

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
				_tickCount++;
			}

			inline void incrementSysTimerCount()
			{
				_sysTimerCount++;
			}

			inline std::uint64_t getTickCount() const
			{
				return _tickCount;
			}

			inline std::uint64_t getSysTimerCount() const
			{
				return _sysTimerCount;
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


			std::uint64_t 	_tickCount = 0u;

			std::uint64_t 	_sysTimerCount = 0u;

			bool insideInterrupt = false;;



		};
	}
}

#pragma once
#include "Interface.hpp"





namespace CppRtos
{
	namespace Port
	{


		constexpr std::uint32_t  FIRST_USER_INTERRUPT_NUMBER  = 16u;

		class Port : public IPort
		{
		public:
			Port()
			{
			}

			~Port()
			{
			}

			bool isInsideInterrupt( void ) const override;

			inline void disableInterrupts( void )  const override;

			inline void enableInterrupts( void )  const override;

			inline void enterCritical(void)  const override;

			inline void exitCritical(void)  const override;

			void vRaiseBASEPRI( void ) const override;

			std::uint32_t ulRaiseBASEPRI( void ) const override;

			inline void vSetBASEPRI(std::uint32_t value) const override;

			inline void memoryBarrier( void ) const override;

			void setupTimerInterrupt( void ) const override;

			void validateInterruptPriority(void) const override;

			bool startScheduler(void) const override;

			void endScheduler(void) const override;


			void startFirstTask(void) const override;

			void enableVFP(void) const override;

			void taskExitError(void) const override;

			void* pxPortInitialiseStack(void* pxTopOfStack, /*TaskFunction_t*/ std::uint32_t pxCode, void* pvParameters) override;

		private:


			inline void vTaskSwitchContext(void);

			inline std::size_t getCurrentTCB(void);



		};
	}
}

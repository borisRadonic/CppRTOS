#pragma once
#include "Interface.hpp"
#include "ArmCM7.hpp"


namespace CppRtos
{
	namespace Port
	{

		constexpr std::uint32_t  FIRST_USER_INTERRUPT_NUMBER  = 16u;

		#define NVIC_INT_CTRL_REG     (*(volatile uint32_t *)0xE000ED04)
		#define NVIC_PENDSVSET_BIT    (1UL << 28UL)

		class Port : public IPort
		{
		public:

			Port()
			{
			}

			~Port()
			{
			}


			inline bool isInsideInterrupt( void ) const override
			{
				return _cpu.isIRQMode();
			}

			inline void yield() const
			{
				 // Set a PendSV to request a context switch.
				NVIC_INT_CTRL_REG |= NVIC_PENDSVSET_BIT;
		        __asm volatile ( "dsb" ::: "memory" );
		        __asm volatile ( "isb" );
			}

			inline void disableInterrupts( void )  const
			{
				ulRaiseBASEPRI();
			}

			inline void enableInterrupts( void ) const override
			{
				vSetBASEPRI(0u);
			}

			inline void enterCritical(void)  const override
			{
				ulRaiseBASEPRI(); //disable interrupts
			}

			inline void exitCritical(void) const override
			{
				enableInterrupts();
			}

			void vRaiseBASEPRI( void ) const override;

			std::uint32_t ulRaiseBASEPRI( void ) const override;

			inline void vSetBASEPRI(std::uint32_t value) const
			{
				__asm volatile
				(
						"   msr basepri, %0 " ::"r" ( value ) : "memory"
				);
			}

			inline void memoryBarrier( void ) const
			{
				__asm volatile ( "" ::: "memory" );
			}

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

			ARMCM7 _cpu;


		};
	}
}

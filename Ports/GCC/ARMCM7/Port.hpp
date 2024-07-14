#pragma once
#include <assert.h>
#include "Interface.hpp"
#include "ArmCM7.hpp"


namespace CppRtos
{
	namespace Port
	{


	/*ARM Cortex-M7*/

	/*NVIC (Nested Vectored Interrupt Controller):
	 The NVIC is a hardware component integrated into ARM Cortex-M processors,
	 providing a means to manage interrupts and exceptions in a system.*/

	/*Specific interrupts can be enabled or disabled using the Interrupt Set-Enable and Interrupt Clear-Enable Registers.*/
	
	/*The NVIC uses a vector table to determine the address of the interrupt service routines (ISRs).
	Each interrupt has an entry in this table pointing to its corresponding ISR.*/

	/*The NVIC provides registers to set or clear the pending status of interrupts.
	 This allows software to manage the state of interrupts programmatically.
	
	The NVIC includes the SysTick timer, a system timer commonly used for creating time delays and generating periodic interrupts.
	
	Interrupt Set-Enable Register (ISER): Used to enable interrupts. Writing a '1' to a bit in this register enables the corresponding interrupt.

	Interrupt Clear-Enable Register (ICER): Used to disable interrupts. Writing a '1' to a bit in this register disables the corresponding interrupt.
	
	Interrupt Set-Pending Register (ISPR): Used to set the pending status of interrupts. Writing a '1' to a bit in this register sets the corresponding interrupt as pending.
	
	Interrupt Clear-Pending Register (ICPR): Used to clear the pending status of interrupts. Writing a '1' to a bit in this register clears the corresponding interrupt from being pending.
	
	Interrupt Priority Registers (IPR): Used to set the priority level of interrupts. Each interrupt has a corresponding priority field in these registers.
	*/



		constexpr std::uint32_t  FIRST_USER_INTERRUPT_NUMBER  = 16u;

		#define MAX_SYSCALL_INTERRUPT_PRIORITY 5u

		/*Interrupt Controll and State Register (ICSR)*/
		#define NVIC_ICSR     (*(volatile uint32_t *) 0xE000ED04)

		#define ICSR_PENDSVSET_BIT    (1UL << 28UL)
		#define ICSR_PENDSVSET_CLR    (1UL << 27UL)

		/*SysTick Control and Status Register (STK_CTRL)
			Bit 0 (ENABLE): Enables the SysTick counter.
							    0 = Counter disabled.
    							1 = Counter enabled.
			Bit 1 (TICKINT): Enables SysTick exception request (interrupt).
							    0 = Counting down to 0 does not generate an interrupt.
    							1 = Counting down to 0 generates an interrupt.
			Bit 2 (CLKSOURCE): Selects the clock source.
    							0 = External clock.
    							1 = Processor clock (usually referred to as core clock).
			Bits [3:15]: Reserved.
			Bit 16 (COUNTFLAG): Returns 1 if timer counted to 0 since the last read of this register.
						    	0 = Timer has not counted to 0.
    							1 = Timer has counted to 0.

			Bits [17:31]: Reserved.
		*/
		#define SYSTICK_CTRL_REG (*((volatile uint32_t *)0xe000e010u))
		#define SYSTICK_LOAD_REG (*((volatile uint32_t *)0xe000e014u))

		/* Macros to configure the SysTick timer. */
		#define SYSTICK_ENABLE            	0x00000001u
		#define SYSTICK_TICKINT           	0x00000002u
		#define SYSTICK_CLKSOURCE_INTERNAL  0x00000004u  /*Internal Clock Flag*/
		#define SYSTICK_COUNTFLAG         	0x00010000u


		/*SysTick Current Value Register (STK_VAL) is used to read or write the current value of the SysTick timer.
		 When the timer reaches zero, it resets to the value held in the SysTick Reload Value Register (STK_LOAD)
		 and optionally generates an interrupt.
		 Bits [23:0] (CURRENT): The current value of the SysTick counter.
		 Bits [31:24]: Reserved.		 
		 */
		#define SYSTICK_CURRENT_VALUE_REG (*((volatile uint32_t *)0xe000e018u))

		/*System Handler Priority Register 2 (SHPR2) allows you to configure the priority of the following system exceptions:
		    SVCall (Supervisor Call): Used for system service calls.
    		PendSV (Pendable Service Call): Used for context switching in an operating system.
    		SysTick (System Tick Timer): Used for the OS tick in an RTOS.
			
		Register Layout
						    Bits [7:0]: Priority of the SVCall exception.
    						Bits [15:8]: Reserved.
    						Bits [23:16]: Reserved.
    						Bits [31:24]: Priority of the SysTick exception.
		*/
		#define NVIC_SHPR2 (*((volatile uint32_t *)0xE000ED1C))

		/*System Handler Priority Register 3 (SHPR3) is used to set the priority for: 
		- PendSV (Pendable Service Call) (Context Switching)
		- SysTick (System Tick Timer)
		- Debug Monitor

		Bits [7:0]: Reserved or implementation-defined.
		Bits [15:8]: Priority of the PendSV exception.
		Bits [23:16]: Priority of the SysTick exception.
		Bits [31:24]: Priority of the Debug Monitor exception.
		*/
		#define NVIC_SHPR3 (*((volatile uint32_t *)0xE000ED20))
		#define SHPR_PRIO_LSHIFT_PENDSV_BITS  16u
		#define SHPR_PRIO_LSHIFT_SYSTICK_BITS 24u

		#define PRIO_BITS 4u
		constexpr std::uint8_t LOWEST_INT_PRIORITY = 15u;

		//Highest interrupt priority that can be used by any ISR
		constexpr std::uint8_t MAX_SYSCALL_INT_PRIORIRY = 5u;

		#define MIN_INTERRUPT_PRIORITY            ( 255UL )

		constexpr std::uint32_t	NVIC_PENDSV_PRI =  ( ( ( uint32_t ) MIN_INTERRUPT_PRIORITY ) << 16UL );
		constexpr std::uint32_t	NVIC_SYSTICK_PRI = ( ( ( uint32_t ) MIN_INTERRUPT_PRIORITY ) << 24UL );
	

		//constexpr std::uint32_t	NVIC_PENDSV_PRI =  (MAX_SYSCALL_INT_PRIORIRY << (8u - PRIO_BITS)) << SHPR_PRIO_LSHIFT_PENDSV_BITS;
		//constexpr std::uint32_t	NVIC_SYSTICK_PRI = (MAX_SYSCALL_INT_PRIORIRY << (8u - PRIO_BITS)) << SHPR_PRIO_LSHIFT_SYSTICK_BITS;
	
		// Define the address of the FPCCR register (SCB_FPCCR)

		#define PFPCCR *(( uint32_t *) 0xE000EF34 )

		// Define the bits for ASPEN (Automatic State Preservation Enable) and LSPEN (Lazy State Preservation Enable)
		#define ASPEN_BIT  (1 << 31)
		#define LSPEN_BIT  (1 << 30)
		#define ASPEN_AND_LSPEN_BITS (ASPEN_BIT | LSPEN_BIT)


		class Port : public IPort
		{
		public:


			union FunctionPointerUnion
			{
			    void (Port::*taskExitError)();
			    std::uint32_t uintRepresentation;
			};

			Port()
			 :_tickCount ( 0u )
			 , _sysTimerCount( 0u )
			{
			}

			~Port()
			{
			}

			inline bool isInsideInterrupt( void ) const override
			{
				return _cpu.isIRQMode();
			}

			inline void yield()
			{
				 // Set a PendSV to request a context switch.
				enterCritical();
				NVIC_ICSR = ICSR_PENDSVSET_BIT;
				exitCritical();

				//NVIC_ICSR |= ICSR_PENDSVSET_BIT;
		        //__asm volatile ( "dsb" ::: "memory" );
		        //__asm volatile ( "isb" );
			}

			inline void disableInterrupts( void )  const
			{
				raiseGetBASEPRI();
			}

			inline void enableInterrupts( void ) const override
			{
				vSetBASEPRI(0u);
			}

			inline void enterCritical(void)  override
			{
				_nestingCounter++;
				raiseGetBASEPRI(); //disable interrupts
			}

			inline void exitCritical(void) override
			{
				if( _nestingCounter != 0 )
				{
					//assert( _nestingCounter != 0)
					//while(true) {};
				}
				
				_nestingCounter--;
				if( _nestingCounter == 0u )
				{
					enableInterrupts();
				}
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
		
			inline void memoryBarrier( void ) const
			{
				__asm volatile ( "" ::: "memory" );
			}

			void setupTimerInterrupt( void ) const override;

			void validateInterruptPriority(void) const override;

			void startScheduler(void) override;

			void endScheduler(void) const override;

			void startFirstTask(void) const override;

			void enableVFP(void) const override;

			void taskExitError(void) override;

			void* initialiseStack(void* pxTopOfStack, void* pvParameters) override;


		private:

			void raiseBASEPRI( void ) const;

			std::uint32_t raiseGetBASEPRI( void ) const;

			inline void vSetBASEPRI(std::uint32_t value) const
			{
				__asm volatile
				(
						"   msr basepri, %0 " ::"r" ( value ) : "memory"
				);
			}

 			std::uint64_t 	_tickCount = 0u;

        		std::uint64_t 	_sysTimerCount = 0u;

			std::uint32_t _nestingCounter = 0u;

			ARMCM7 _cpu;

			void setPrivilegedMode(void);
		};
	}
}

#include "../Include/Port.hpp"
#include "../Include/Config.hpp"



	extern "C" void vPortSVCHandler( void );
	extern "C" void xPortPendSVHandler( void );


#define MAX_SYSCALL_INTERRUPT_PRIORITY 5u

	volatile void *pxCurrentTCB  = nullptr;

	extern "C" void vTaskSwitchContext(void) __attribute__((section("privileged_functions")));


	void vTaskSwitchContext()
	{
		int a = 0;
		a++;
	}


	void vPortSVCHandler( void )
	{
	    __asm volatile (
	        "   ldr r3, pxCurrentTCBConst2      \n" /* Restore the context. */
	        "   ldr r1, [r3]                    \n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
	        "   ldr r0, [r1]                    \n" /* The first item in pxCurrentTCB is the task top of stack. */
	        "   ldmia r0!, {r4-r11, r14}        \n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
	        "   msr psp, r0                     \n" /* Restore the task stack pointer. */
	        "   isb                             \n"
	        "   mov r0, #0                      \n"
	        "   msr basepri, r0                 \n"
	        "   bx r14                          \n"
	        "                                   \n"
	        "   .align 4                        \n"
	        "pxCurrentTCBConst2: .word pxCurrentTCB             \n"
	        );
	}

	void pendSVHandler( void )
	{
		/* This is a naked function. */


		__asm volatile
		(
			"   mrs r0, psp                         \n" /*the current location of the psp (the stack that was in use prior to exception entry) is loaded into r0*/
			"   isb                                 \n" /*Instruction Synchronization Barrier) flushes the instruction pipeline guaranteeing any instruction which follows will be re-fetched*/
			"                                       \n"
			"   ldr r3, pxCurrentTCBConst           \n" /* Get the location of the current TCB. */
			"   ldr r2, [r3]                        \n" /*The value of pxCurrentTCB gets loaded into r2*/
			"                                       \n"
			"   tst r14, #0x10                      \n" /* Is the task using the FPU context?  If so, push high vfp registers. */
			"   it eq                               \n"
			"   vstmdbeq r0!, {s16-s31}             \n" /* As soon as an FPU instruction is used an additional 132 bytes will be pushed on the stack*/
			"                                       \n"
			"   stmdb r0!, {r4-r11, r14}            \n" /* Save the core registers. */
			"   str r0, [r2]                        \n" /* Save the new top of stack into the first member of the TCB. */
			"                                       \n" /* First member of TCB structure points to the location of the last item placed on the tasks stack */
			"   stmdb sp!, {r0, r3}                 \n"
			"   mov r0, %0                          \n"
			"   cpsid i                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 workaround. */
			"   msr basepri, r0                     \n"
			"   dsb                                 \n"
			"   isb                                 \n"
			"   cpsie i                             \n" /* ARM Cortex-M7 r0p1 Errata 837070 workaround. */
			"   bl vTaskSwitchContext               \n"
			"   mov r0, #0                          \n"
			"   msr basepri, r0                     \n" /*Enable all interrupts by resetting basepri to 0*/
			"   ldmia sp!, {r0, r3}                 \n" /*The initial values of the argument registers (r0-r3) are restored by popping them off the stack*/
			"                                       \n"
			"   ldr r1, [r3]                        \n" /* The first item in pxCurrentTCB is the task top of stack. */
			"   ldr r0, [r1]                        \n"
			"                                       \n"
			"   ldmia r0!, {r4-r11, r14}            \n" /* Pop the core registers. */
			"                                       \n"
			"   tst r14, #0x10                      \n" /* Is the task using the FPU context?  If so, pop the high vfp registers too. */
			"   it eq                               \n"
			"   vldmiaeq r0!, {s16-s31}             \n"
			"                                       \n"
			"   msr psp, r0                         \n"
			"   isb                                 \n"
			"                                       \n"
			#ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata workaround. */
				#if WORKAROUND_PMU_CM001 == 1
					"           push { r14 }                \n"
					"           pop { pc }                  \n"
				#endif
			#endif
			"                                       \n"
			"   bx r14                              \n"
			"                                       \n"
			"   .align 4                            \n"
			"pxCurrentTCBConst: .word pxCurrentTCB  \n"
			::"i" ( MAX_SYSCALL_INTERRUPT_PRIORITY )
		);
	}



namespace CppRtos
{
	namespace Port
	{


		bool Port::isInsideInterrupt( void ) const
		{
			std::uint32_t currInt(0u);
		    __asm volatile ( "mrs %0, ipsr" : "=r" ( currInt )::"memory" );
		    return( 0u != currInt );
		}

		inline void Port::disableInterrupts( void )  const
		{
			ulRaiseBASEPRI();
		}

		inline void Port::enableInterrupts( void )  const
		{
			vSetBASEPRI(0u);
		}

		inline void Port::enterCritical(void)  const
		{
			ulRaiseBASEPRI(); //disable interrupts
		}

		inline void Port::exitCritical(void)  const
		{
			enableInterrupts();
		}

		void Port::vRaiseBASEPRI( void ) const
		{
			std::uint32_t basePri(0u);
			__asm volatile
			(
				"   mov %0, %1                                              \n" \
				"   cpsid i                                                 \n" \
				"   msr basepri, %0                                         \n" \
				"   isb                                                     \n" \
				"   dsb                                                     \n" \
				"   cpsie i                                                 \n" \
				: "=r" ( basePri ) : "i" ( MAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
			);
		}

		std::uint32_t Port::ulRaiseBASEPRI( void ) const
		{
			std::uint32_t origBasePri(0u);
			std::uint32_t newBasePri(0u);
			__asm volatile
			(
				"   mrs %0, basepri                                         \n" \
				"   mov %1, %2                                              \n" \
				"   cpsid i                                                 \n" \
				"   msr basepri, %1                                         \n" \
				"   isb                                                     \n" \
				"   dsb                                                     \n" \
				"   cpsie i                                                 \n" \
				: "=r" ( origBasePri ), "=r" ( newBasePri ) : "i" ( MAX_SYSCALL_INTERRUPT_PRIORITY ) : "memory"
			);
			return origBasePri;
		}




		inline void Port::vSetBASEPRI(std::uint32_t value) const
		{
			__asm volatile
			(
					"   msr basepri, %0 " ::"r" ( value ) : "memory"
			);
		}

		inline void Port::memoryBarrier( void ) const
		{
			__asm volatile ( "" ::: "memory" );
		}

		void Port::setupTimerInterrupt( void ) const
		{
			/* Stop and clear the SysTick. */
		//portNVIC_SYSTICK_CTRL_REG = 0UL;
		// portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

		 /* Configure SysTick to interrupt at the requested rate. */
		//	    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
		//	    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
		}

//		void xPortSysTickHandler( void )
//		{
//			//disable interrupts
//			ulRaiseBASEPRI();
//			 /* Increment the RTOS tick. */
//			 if( xTaskIncrementTick() != pdFALSE )
//			 {
//				 /* A context switch is required.  Context switching is performed in  the PendSV interrupt.  Pend the PendSV interrupt. */
//			     portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
//			 }
//			 vSetBASEPRI(0u);
//		}



		void Port::validateInterruptPriority(void) const
		{

		}


		bool Port::startScheduler(void) const
		{


		 /* Make PendSV and SysTick the lowest priority interrupts, and make SVCall
			 * the highest priority. */
			//portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
			//portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;
			//portNVIC_SHPR2_REG = 0;

		 /* Start the timer that generates the tick ISR.  Interrupts are disabled
			 * here already. */
			//vPortSetupTimerInterrupt();

			/* Enable VFPd  */
			this->enableVFP();

		 /* Lazy save always. */
			// *( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

		   /* Start the first task. */
			this->startFirstTask();


			/* Should never get here as the tasks will now be executing!  Call the task
			* exit error function to prevent compiler warnings about a static function
			* not being called in the case that the application writer overrides this
			* functionality by defining configTASK_RETURN_ADDRESS.  Call
			* vTaskSwitchContext() so link time optimisation does not remove the
			* symbol. */
			//vTaskSwitchContext();
			//prvTaskExitError();
			/* Should not get here! */

			return false;
		}

		void Port::endScheduler(void) const
		{
			//assert();
		}

		void Port::startFirstTask(void) const
		{
			__asm volatile (
				" ldr r0, =0xE000ED08   \n" /* Use the NVIC offset register to locate the stack. */
				" ldr r0, [r0]          \n"
				" ldr r0, [r0]          \n"
				" msr msp, r0           \n" /* Set the msp back to the start of the stack. */
				" mov r0, #0            \n" /* Clear the bit that indicates the FPU is in use, see comment above. */
				" msr control, r0       \n"
				" cpsie i               \n" /* Globally enable interrupts. */
				" cpsie f               \n"
				" dsb                   \n"
				" isb                   \n"
				" svc 0                 \n" /* System call to start first task. */
				" nop                   \n"
				" .ltorg                \n"
				);
		}

		void Port::enableVFP(void)  const
		{
			__asm volatile
			(
				"   ldr.w r0, =0xE000ED88       \n" /* The FPU enable bits are in the CPACR. */
				"   ldr r1, [r0]                \n"
				"                               \n"
				"   orr r1, r1, #( 0xf << 20 )  \n" /* Enable CP10 and CP11 coprocessors, then save back. */
				"   str r1, [r0]                \n"
				"   bx r14                      \n"
				"   .ltorg                      \n"
			);
		}

		void Port::taskExitError(void)  const
		{
			disableInterrupts();
			while( true) {};
		}

		void* Port::pxPortInitialiseStack(void* pxTopOfStack, /*TaskFunction_t*/ std::uint32_t pxCode, void* pvParameters)
		{
//			 /* Simulate the stack frame as it would be created by a context switch interrupt. */
//
//			 /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts, and to ensure alignment. */
//			 pxTopOfStack--;
//
//			 *pxTopOfStack = portINITIAL_XPSR;                                    /* xPSR */
//			 pxTopOfStack--;
//			 *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK; /* PC */
//			 pxTopOfStack--;
//			 *pxTopOfStack = ( StackType_t ) portTASK_RETURN_ADDRESS;             /* LR */
//
//			    /* Save code space by skipping register initialisation. */
//			 pxTopOfStack -= 5;                            /* R12, R3, R2 and R1. */
//			 *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 */
//
//			    /* A save method is being used that requires each task to maintain its
//			     * own exec return value. */
//			 pxTopOfStack--;
//			 *pxTopOfStack = portINITIAL_EXC_RETURN;
//
//			 pxTopOfStack -= 8; /* R11, R10, R9, R8, R7, R6, R5 and R4. */

			 return pxTopOfStack;
		}


		/*Private functions*/

		inline void Port::vTaskSwitchContext(void)
		{
		}

		inline std::size_t Port::getCurrentTCB(void)
		{
			/*get cureent TCB*/

			return 0U;
		}

	}
}

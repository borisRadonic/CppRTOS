#include "../Include/Port.hpp"
#include "../Include/Config.hpp"
#include "Kernel.hpp"


	void pendSVHandler(void) __attribute__((naked));
	void SVCHandler( void )  __attribute__((naked));
	
	extern "C" void taskSwitchContext(void);

	void taskSwitchContext()
	{
		/*call kernel function*/
		Kernel* pKernel = KernelFactory::getInstance().getKernel();
		if( kernell != nullptr )
		{
			kernel->selectHighestPriorityTask();
		}	
	}

	

	volatile void *pxCurrentTCB  = nullptr;

	extern "C" void vTaskSwitchContext(void) __attribute__((section("privileged_functions")));


	void vTaskSwitchContext()
	{
		int a = 0;
		a++;
	}


	void SVCHandler( void )
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

	void pendSVHandler(void)
	{
	    __asm volatile
	    (
	    "   mrs r0, psp                         \n"
	    "   isb                                 \n"
	    "                                       \n"
	    "   ldr r3, currentTaskDataAddr         \n" /* Get the location of the currentTaskData variable. */
	    "   ldr r2, [r3]                        \n"
	    "                                       \n"
	    "   tst r14, #0x10                      \n" /* Is the task using the FPU context? If so, push high vfp registers. */
	    "   it eq                               \n"
	    "   vstmdbeq r0!, {s16-s31}             \n"
	    "                                       \n"
	    "   stmdb r0!, {r4-r11, r14}            \n" /* Save the core registers. */
	    "   str r0, [r2]                        \n" /* Save the new top of stack into the first member of the TCB. */
	    "                                       \n"
	    "   stmdb sp!, {r0, r3}                 \n"
	    "   mov r0, %0                          \n"
	    "   cpsid i                             \n" /* Errata workaround. */
	    "   msr basepri, r0                     \n"
	    "   dsb                                 \n"
	    "   isb                                 \n"
	    "   cpsie i                             \n" /* Errata workaround. */
	    "   bl taskSwitchContext                \n"
	    "   mov r0, #0                          \n"
	    "   msr basepri, r0                     \n"
	    "   ldmia sp!, {r0, r3}                 \n"
	    "                                       \n"
	    "   ldr r1, [r3]                        \n" /* The first item in currentTaskData is the task top of stack. */
	    "   ldr r0, [r1]                        \n"
	    "                                       \n"
	    "   ldmia r0!, {r4-r11, r14}            \n" /* Pop the core registers. */
	    "                                       \n"
	    "   tst r14, #0x10                      \n" /* Is the task using the FPU context? If so, pop the high vfp registers too. */
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
	    "currentTaskDataAddr: .word currentTaskData\n"
	    :: "i" (configMAX_SYSCALL_INTERRUPT_PRIORITY)
	    );
	}


namespace CppRtos
{
	namespace Port
	{
		void Port::raiseBASEPRI( void ) const
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

		std::uint32_t Port::raiseGetBASEPRI( void ) const
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

		void Port::setupTimerInterrupt( void ) const
		{
			SYSTICK_CTRL_REG 			= 0U; // Stop SysTick.
			SYSTICK_CURRENT_VALUE_REG 	= 0U; // Reset the current value of the SysTick counter to 0
		
		 	// Configure SysTick
			SYSTICK_LOAD_REG =  ((CPU_CLOCK_HZ / Settings::TICK_RATE_HZ ) - 1u);

			// Internal Clock, Enable SysTick Interrupt and Enable the SysTick counter
		    SYSTICK_CTRL_REG = ( SYSTICK_CLKSOURCE_INTERNAL | SYSTICK_TICKINT | SYSTICK_ENABLE );
		}

		void xPortSysTickHandler( void )
		{
			//disable interrupts
			//ulRaiseBASEPRI();
			 /* Increment the RTOS tick. */
			 //if( xTaskIncrementTick() != pdFALSE )
			 //{
			//	 /* A context switch is required.  Context switching is performed in  the PendSV interrupt.  Pend the PendSV interrupt. */
			 //    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
			 //}
			 //vSetBASEPRI(0u);
		}


		void Port::validateInterruptPriority(void) const
		{
		}

		void Port::startScheduler(void) const
		{
		 	// Make PendSV and SysTick the lowest priority interrupts
			NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 16)) | NVIC_PENDSV_PRI;
        	NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 24)) | NVIC_SYSTICK_PRI;
			
			// Make SVCall the highest priority interrupt. 
			NVIC_SHPR2 = 0u;

		 	// Start the timer for the tick ISR.
			this->setupTimerInterrupt();

			// Enable VFP
			this->enableVFP();

		 /* Lazy save always. */
			// *( portFPCCR ) |= portASPEN_AND_LSPEN_BITS;

		   /* Start the first task. */
			this->startFirstTask();


			/* Should never get here as the tasks will now be executing!  Call the task
			* exit error function to prevent compiler warnings about a static function
			* not being called in the case that the application writer overrides this
			* functionality by defining configTASK_RETURN_ADDRESS.  Call
			* vTaskSwitchContext() so link time optimisation does not remove the symbol. */
			//vTaskSwitchContext();
			//prvTaskExitError();
			/* Should not get here! */
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

		/*
		The xPSR (Program Status Register) is a critical register in ARM Cortex-M processors, including the Cortex-M7. 
		It combines three separate status registers: the Application Program Status Register (APSR),
		the Interrupt Program Status Register (IPSR), and the Execution Program Status Register (EPSR).

		Components of xPSR
		APSR (Application Program Status Register):

		N (Negative) Flag: Indicates that the result of the last arithmetic operation was negative.
		Z (Zero) Flag: Indicates that the result of the last arithmetic operation was zero.
		C (Carry) Flag: Indicates that the last arithmetic operation resulted in a carry or borrow.
		V (Overflow) Flag: Indicates that the last arithmetic operation resulted in an overflow.
		IPSR (Interrupt Program Status Register):

		ISR Number: Contains the number of the current exception or interrupt being processed. If no interrupt is being processed, this field is zero.
		EPSR (Execution Program Status Register):

		Thumb State Bit (T): Indicates the processor state. It should always be set to 1 in Cortex-M processors, indicating that the processor is executing Thumb instructions.
		ICI/IT (If-Then Execution State Bits): Control conditional execution of instructions within an IT (If-Then) block.
		*/
		void* Port::pxPortInitialiseStack(void* pxTopOfStack, /*TaskFunction_t*/ std::uint32_t pxCode, void* pvParameters)
		{
			 /* Simulate the stack frame as it would be created by a context switch interrupt. */

			 /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts, and to ensure alignment. */
			 pxTopOfStack--;

			 *pxTopOfStack = 0x01000000;   /* xPSR: Set the Thumb state bit (T bit) */
			 pxTopOfStack--;

			/* Adress Mask 0xfffffffeUL For strict compliance with the Cortex-M spec the task start
   			address should have bit-0 clear, as it is loaded into the PC on exit from an ISR. */

			
			 *pxTopOfStack = ( ( StackType_t ) pxCode ) & 0xfffffffeUL; /* PC */
			 pxTopOfStack--;

			/*Return Address*/
			 *pxTopOfStack = ( StackType_t ) prvTaskExitError;             /* LR */
   		       
			 pxTopOfStack -= 5;                            /* R12, R3, R2 and R1. */
			 *pxTopOfStack = ( StackType_t ) pvParameters; /* R0 */

			 pxTopOfStack--;
			 *pxTopOfStack = pvParameters; /* R0: Argument - return value*/

			 pxTopOfStack -= 8; /* R11, R10, R9, R8, R7, R6, R5 and R4. */
			 return pxTopOfStack;
		}


	}
}

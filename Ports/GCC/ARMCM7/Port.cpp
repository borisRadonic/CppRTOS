#include "Port.hpp"
#include "Config.hpp"
#include "Kernel.hpp"

	struct CURRENT_TCB
	{
		void* currentStackPtr = nullptr;
	};

	static CURRENT_TCB tcb;

	CURRENT_TCB *ptrCurrentTask = &tcb;

	extern "C" void SVC_Handler( void );

	extern "C" void PendSV_Handler( void );

	extern "C" void taskSwitchContext(void) __attribute__((section("privileged_functions")));

	void taskSwitchContext()
	{
		/*call kernel function*/
		CppRtos::Kernel* pKernel = CppRtos::KernelFactory::getInstance().getKernel();
		//pKernel->
		//pKernel->enterCritical();
		if( pKernel != nullptr )
		{
			//store updated stack pointer
			CppRtos::TaskData* ptrTaskData = pKernel->getCurrentTask();
			ptrTaskData->setCurrentStackPtr( ptrCurrentTask->currentStackPtr );

			pKernel->selectHighestPriorityTask();

			if( pKernel->getCurrentTask()->getCurrentStackPtr() != ptrCurrentTask->currentStackPtr )
			{
				//get new stack pointer and store to ptrCurrentTask
				ptrTaskData = pKernel->getCurrentTask();			
				ptrCurrentTask->currentStackPtr = ptrTaskData->getCurrentStackPtr();
			}
		}
		//pKernel->_port.exitCritical();
	}


	std::uint32_t currentTaskDataAddr = 0;


	union ConvertVoidPtrUin32
	{
		void* dataVoid;
		std::uint32_t uint32;
	};


	void SVC_Handler( void )
	{
		CppRtos::Kernel* pKernel = CppRtos::KernelFactory::getInstance().getKernel();
		ConvertVoidPtrUin32 convert;
		convert.dataVoid = static_cast<void*> (pKernel->getCurrentTask());
		currentTaskDataAddr = convert.uint32;
		ptrCurrentTask->currentStackPtr = convert.dataVoid;

	  	__asm volatile
	    (
        	"   ldr r3, =currentTaskDataAddr    \n" /* Load the address of currentTaskDataAddr. */
	  	);
        __asm volatile
		(
			"   ldr r1, [r3]                    \n" /* Use currentTaskDataAddr to get the current task data address. */
		);
        __asm volatile
		(
			"   ldr r0, [r1]                    \n" /* The first item in the current task data is the task top of stack. */
		);

		__asm volatile
		(
        "   ldmia r0!, {r4-r11, r14}        \n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
        "   msr psp, r0                     \n" /* Restore the task stack pointer. */
        "   isb                             \n" /* Instruction Synchronization Barrier. */
        "   mov r0, #0                      \n" /* Clear the BASEPRI register. */
        "   msr basepri, r0                 \n" /* Restore interrupts. */
		);
		__asm volatile
		(
        "   bx r14                          \n" /* Return from the exception handler. */
        "                                   \n"
        "   .align 4                        \n"
    	);

	}

	extern "C" void HardFault_Handler(void)
	{
		while (1);  // Infinite loop to catch HardFault
	}

	extern "C" void MemManage_Handler(void)
	{
		while (1);  // Infinite loop to catch Memory Management Fault
	}

	extern "C" void BusFault_Handler(void)
	{
		while (1);  // Infinite loop to catch Bus Fault
	}

	extern "C" void UsageFault_Handler(void)
	{
		while (1);  // Infinite loop to catch Usage Fault
	}



	void PendSV_Handler(void) 
	{		
		 __asm volatile
		 (
		     "   mrs r0, psp                         \n" // Move the current value of the Process Stack Pointer (PSP) into r0
		     "   isb                                 \n" // Instruction Synchronization Barrier to ensure subsequent instructions use updated values

		     "   ldr r3, ptrCurrentTaskConst         \n" // Load the address of ptrCurrentTask into r3
		     "   ldr r2, [r3]                        \n" // Load the value of pxCurrentTCB (i.e., the current TCB) into r2

		     "   tst r14, #0x10                      \n" // Test if bit 4 of r14 (EXC_RETURN) is set (indicating FPU context)
		     "   it eq                               \n" // If the zero flag is set (result of tst is zero), execute the next instruction
		     "   vstmdbeq r0!, {s16-s31}             \n" // If using FPU context, store the high VFP registers (s16-s31) and decrement r0
		    
		     "   stmdb r0!, {r4-r11, r14}            \n" // Store multiple registers (r4-r11, r14) and decrement r0
		     "   str r0, [r2]                        \n" // Store the updated stack pointer (r0) into the current TCB (pointed to by r2)
		    
		     "   stmdb sp!, {r0, r3}                 \n" // Store r0 and r3 on the stack and decrement SP
		     "   mov r0, %0                          \n" // Move the value of MAX_SYSCALL_INTERRUPT_PRIORITY into r0
		     "   msr basepri, r0                     \n" // Move the value of r0 into the BASEPRI register (set interrupt priority)
		     "   dsb                                 \n" // Data Synchronization Barrier to ensure all memory accesses complete
		     "   isb                                 \n" // Instruction Synchronization Barrier to ensure subsequent instructions use updated values
		     "   bl taskSwitchContext               \n"  // Branch to the taskSwitchContext function (perform a context switch)

			  "   .align 4                            \n" // Align the next data on a 4-byte boundary
			  ::"i" ( MAX_SYSCALL_INTERRUPT_PRIORITY )
		 );

 		 uint32_t r0_value = 0u;
 		 __asm volatile
		 (
		    "   mov r0, #0                          \n" // Clear r0
		    "   msr basepri, r0                     \n" // Clear the BASEPRI register (reset interrupt priority)
		    "   ldmia sp!, {r0, r3}                 \n" // Load multiple registers (r0 and r3) from the stack and increment SP
		    
		    "   ldr r1, [r3]                        \n" // Load the value at the address in r3 (i.e., the new TCB) into r1
		    "   ldr r0, [r1]                        \n" // Load the stack pointer of the new task from the new TCB (pointed to by r1) into r0
			: "=r" (r0_value)                         // Output operand to get the value of r0
			:                                         // No input operands
			: "r0", "r1", "r3", "memory"              // Clobbered registers
			);

			if (r0_value == 0)
			{
				return;
			}

		  __asm volatile
		 (		  		    
		    "   ldmia r0!, {r4-r11, r14}            \n" // Load multiple registers (r4-r11, r14) from the stack and increment r0
		    
		    "   tst r14, #0x10                      \n" // Test if bit 4 of r14 (EXC_RETURN) is set (indicating FPU context)
		    "   it eq                               \n" // If the zero flag is set (result of tst is zero), execute the next instruction
		    "   vldmiaeq r0!, {s16-s31}             \n" // If using FPU context, load the high VFP registers (s16-s31) and increment r0
		    
		    "   msr psp, r0                         \n" // Move the updated stack pointer (r0) into the Process Stack Pointer (PSP)
		    "   isb                                 \n" // Instruction Synchronization Barrier to ensure subsequent instructions use updated values
		    
		    #if WORKAROUND_PMU_CM001 == 1
			"           push { r14 }                \n" // Push r14 onto the stack
			"           pop { pc }                  \n" // Pop the value from the stack into the Program Counter (pc)
		    #endif
		
		    "   bx r14                              \n" // Branch to the address in r14 (return from exception)
		    
		    "   .align 4                            \n" // Align the next data on a 4-byte boundary
		    "ptrCurrentTaskConst: .word ptrCurrentTask  \n" // Define a word containing the address of pxCurrentTCB
		    ::"i" ( MAX_SYSCALL_INTERRUPT_PRIORITY )
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


extern "C" uint32_t SystemCoreClock;

		void Port::setupTimerInterrupt( void ) const
		{
			SYSTICK_CTRL_REG 			= 0U; // Stop SysTick.
			SYSTICK_CURRENT_VALUE_REG 	= 0U; // Reset the current value of the SysTick counter to 0
		
		 	// Configure SysTick
			SYSTICK_LOAD_REG =  ((SystemCoreClock / Settings::TICK_RATE_HZ ) - 1u);

			// Internal Clock, Enable SysTick Interrupt and Enable the SysTick counter
		    SYSTICK_CTRL_REG = ( SYSTICK_CLKSOURCE_INTERNAL | SYSTICK_TICKINT | SYSTICK_ENABLE );
		}

		extern "C" void SysTick_Handler( void )
		{
			CppRtos::Kernel* pKernel = CppRtos::KernelFactory::getInstance().getKernel();
			pKernel->disableInterrupts();
			pKernel->incrementTickCount();
			NVIC_ICSR = ICSR_PENDSVSET_BIT;
			pKernel->enableInterrupts();
		}

		void Port::validateInterruptPriority(void) const
		{
		}

		void Port::startScheduler(void)
		{
		 	// Make PendSV and SysTick the lowest priority interrupts
			NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 16)) | NVIC_PENDSV_PRI;
        	NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 24)) | NVIC_SYSTICK_PRI;
			
			// Make SVCall the highest priority interrupt. 
			NVIC_SHPR2 = 0u;

		 	// Start the timer for the tick ISR.
			this->setupTimerInterrupt();

			setPrivilegedMode();

			 /* Lazy save FPU registers */
			 PFPCCR |= ASPEN_AND_LSPEN_BITS;

			this->startFirstTask();
			/*It should never get here*/

			
		}

		void Port::endScheduler(void) const
		{
			//assert();
		}

		void Port::startFirstTask(void) const
		{
		__asm volatile (
        " ldr r0, =0xE000ED08   \n" /* Load the address of the VTOR register. */
        " ldr r0, [r0]          \n" /* Dereference to get the address of the vector table. */
        " ldr r0, [r0]          \n" /* Dereference again to get the initial stack pointer value. */
		);
		__asm volatile (
        " msr msp, r0           \n" /* Set the MSP back to the start of the stack. */
        " mov r0, #0            \n" /* Clear the CONTROL register (set to privileged mode, use MSP). */
        " msr control, r0       \n"
        " cpsie i               \n" /* Enable interrupts. */
        " cpsie f               \n" /* Enable fault interrupts. */
        " dsb                   \n" /* Data Synchronization Barrier. */
        " isb                   \n" /* Instruction Synchronization Barrier. */
        " svc 0                 \n" /* System call to start first task. */
        " nop                   \n" /* No operation (placeholder). */
        " .ltorg                \n" /* Literal pool location directive. */
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

		void Port::taskExitError(void)
		{
			disableInterrupts();
			while( true) {};
		}


		extern "C" void callFirstTime(void)
		{
		 	CppRtos::Kernel* pKernel = CppRtos::KernelFactory::getInstance().getKernel();
		 	TaskData* ptrTaskData = pKernel->getCurrentTask();
			if( ptrTaskData != nullptr )
			{
				ITask* ptrTaskInterface = ptrTaskData->getTaskInterfacePtr();
				if( ptrTaskInterface != nullptr )
				{
					return ptrTaskInterface->run();
				}
			}			
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
		void* Port::initialiseStack(void* pxTopOfStack, void* pvParameters )
		{
			 /* Simulate the stack frame as it would be created by a context switch interrupt. */

			 std::uint32_t* topOfStack = static_cast<std::uint32_t*>(pxTopOfStack);

			 /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts, and to ensure alignment. */
			 topOfStack--;

			 *topOfStack = 0x01000000;   /* xPSR: Set the Thumb state bit (T bit) */
			 topOfStack--;

			/* Adress Mask 0xfffffffeUL For strict compliance with the Cortex-M spec the task start
   			address should have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
			*topOfStack =  (uint32_t) callFirstTime & 0xfffffffeUL; // PC;
			 topOfStack--;

			 void (Port::*funcPtr)() = &Port::taskExitError;
		     // Convert the member function pointer to uintptr_t
			 FunctionPointerUnion funcUnion;
			 funcUnion.taskExitError = funcPtr;
     		// Get the integer representation of the function pointer
			uint32_t funcPtrInt32 = funcUnion.uintRepresentation;

			/*Return Address*/
			 *topOfStack = funcPtrInt32;             /* LR */   		       
			 topOfStack -= 5;                        /* skip R12, R3, R2 and R1. */

			 *topOfStack = 0u; /* R0: Argument - return value -not used at the moment*/
			 topOfStack--;

			 /*R11*/
			 *topOfStack = 0xfffffffdu;
			  topOfStack--;
			 
			 /*R10*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			 /*R9*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			 /*R8*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			 /*R7*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			 /*R6*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			/*R5*/
			 *topOfStack = 0x00000000u;
			 topOfStack--;

			/*R4*/
			 *topOfStack = 0x00000000u;
			  topOfStack--;
			 return static_cast<void*>(topOfStack);
		}


		void Port::setPrivilegedMode()
		{
			__asm volatile (
				" mrs r0, control  \n" /* Read the current value of the CONTROL register */
				" bic r0, r0, #1   \n" /* Clear the least significant bit to select privileged mode */
				" msr control, r0  \n" /* Write the modified value back to the CONTROL register */
				" isb              \n" /* Ensure the change takes effect immediately */
			);
		}

	
	}
}

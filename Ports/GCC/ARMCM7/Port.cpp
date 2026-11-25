<<<<<<< HEAD
#include <cstdint>
=======
>>>>>>> d600e6b85ae67ba2e38a9177d457913ab703c81a
#include <cassert>
#include "Port.hpp"
#include "Config.hpp"
#include "KernelFactory.hpp"
#include "Kernel.hpp"

	static CppRtos::Port::Port* gPtrPort = nullptr;
	static CppRtos::Kernel* gPtrKernel = nullptr;

	struct CURRENT_TCB
	{
		void* currentStackPtr = nullptr;
	};

	static CURRENT_TCB tcb;

	CURRENT_TCB *ptrCurrentTask = &tcb;

	extern "C" void startInitialTask( void );

	extern "C" void setPrivilegedMode( void );

	extern "C" void taskSwitchContext(void) noexcept/* __attribute__((section("privileged_functions")))*/;

	extern "C" std::uint32_t setInterruptPriorityAndGetOriginal( void );

	extern "C" void setInterruptBasePriority( std::uint32_t );

	extern "C" void enableVFP( void );

	/* Called if a task's run() function returns (address placed in the initial LR of each task). */
	extern "C" void taskReturnedHandler( void );


	void taskSwitchContext() noexcept
	{
		/*call kernel function*/
		if( gPtrPort != nullptr )
		{			
			//store updated stack pointer
			CppRtos::TaskData* ptrTaskData = gPtrPort->getCurrentTask();
			ptrTaskData->setCurrentStackPtr( ptrCurrentTask->currentStackPtr );

			gPtrPort->selectHighestPriorityTask();

			if( gPtrPort->getCurrentTask()->getCurrentStackPtr() != ptrCurrentTask->currentStackPtr )
			{
				//get new stack pointer and store to ptrCurrentTask
				ptrTaskData = gPtrPort->getCurrentTask();
				ptrCurrentTask->currentStackPtr = ptrTaskData->getCurrentStackPtr();
			}
		}
	}

	std::uint32_t currentTaskDataAddr = 0;

	union ConvertVoidPtrUin32
	{
		void* dataVoid;
		std::uint32_t uint32;
	};

	
namespace CppRtos
{
	namespace Port
	{

		extern "C" uint32_t SystemCoreClock;

		void Port::disableInterrupts( void )  const
		{
			setInterruptPriorityAndGetOriginal();
		}

		void Port::enableInterrupts( void ) const
		{
			setInterruptBasePriority(0u);
		}

		[[nodiscard]] CppRtos::TaskData* Port::getCurrentTask()
		{
			return ptrKernel->getCurrentTask();
		}

		void Port::selectHighestPriorityTask()
		{
			ptrKernel->selectHighestPriorityTask();
		}

		void Port::tick()
		{
			ptrKernel->tick();
		}

		// Not 'inline': these are called from other translation units (Kernel, Mutex, Semaphore, ...).
		void Port::enterCritical()
		{
			// Mask kernel-aware interrupts first, then update the nesting counter,
			// so the counter itself is only modified inside the critical section.
			setInterruptPriorityAndGetOriginal();
			nestingCounter++;
		}

		void Port::exitCritical()
		{
			assert( nestingCounter != 0u );
			if( nestingCounter == 0u )
			{
				// Unbalanced exitCritical(): do not underflow the counter.
				return;
			}

			nestingCounter--;
			if( nestingCounter == 0u )
			{
				enableInterrupts();
			}
		}

		void Port::setupTimerInterrupt()
		{
			SYSTICK_CTRL_REG 			= 0U; // Stop SysTick.
			SYSTICK_CURRENT_VALUE_REG 	= 0U; // Reset the current value of the SysTick counter to 0
		 	// Configure SysTick
			SYSTICK_LOAD_REG =  ((SystemCoreClock / Settings::TICK_RATE_HZ ) - 1u);
			// Internal Clock, Enable SysTick Interrupt and Enable the SysTick counter
		    SYSTICK_CTRL_REG = ( SYSTICK_CLKSOURCE_INTERNAL | SYSTICK_TICKINT | SYSTICK_ENABLE );
		}

		inline void setGlobalPointers()
		{
			if( gPtrKernel == nullptr)
			{
				CppRtos::KernelFactory& kernelFact = CppRtos::KernelFactory::getInstance();
				gPtrKernel = kernelFact.getKernel();
			}

			if( gPtrPort == nullptr)
			{
				CppRtos::Port::Port& refPort = gPtrKernel->getPort();
				gPtrPort = &refPort;
			}
		}
		
		extern "C" void SysTick_Handler( void )
		{
			setGlobalPointers();
			// Use the nesting critical section (not raw BASEPRI writes), so kernel functions called
			// from tick() that enter/exit critical sections do not unmask interrupts prematurely.
			gPtrPort->enterCritical();
			gPtrPort->incrementTickCount();
			gPtrPort->tick();
			NVIC_ICSR = ICSR_PENDSVSET_BIT;
			gPtrPort->exitCritical();
		}

		void Port::validateInterruptPriority() const
		{
		}

		void Port::startScheduler()
		{
			// Mask kernel-aware interrupts until the first task runs. Otherwise a SysTick between
			// enabling interrupts and the SVC in startInitialTask would pend PendSV with no valid
			// task context. SVC_Handler clears BASEPRI when it starts the first task.
			disableInterrupts();

		 	// Make PendSV and SysTick the lowest priority interrupts
			NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 16)) | NVIC_PENDSV_PRI;
        	NVIC_SHPR3 = (NVIC_SHPR3 & ~(0xFF << 24)) | NVIC_SYSTICK_PRI;
			
			// Make SVCall the highest priority interrupt. 
			NVIC_SHPR2 = 0u;

		 	// Start the timer for the tick ISR.
			Port::setupTimerInterrupt();

			setPrivilegedMode();

			 /* Lazy save FPU registers */
			 PFPCCR |= ASPEN_AND_LSPEN_BITS;

			CppRtos::Port::Port::startFirstTask();
			/*It should never get here*/
		}

		void Port::endScheduler() const
		{
			//assert();
		}

		void Port::startFirstTask()
		{
			CppRtos::Kernel* pKernel = CppRtos::KernelFactory::getInstance().getKernel();
			ConvertVoidPtrUin32 convert = {};
			convert.dataVoid = static_cast<void*> (pKernel->getCurrentTask());
			currentTaskDataAddr = convert.uint32;
			ptrCurrentTask->currentStackPtr = convert.dataVoid;
			startInitialTask();
		}

		void Port::taskExitError()
		{
			disableInterrupts();
			while( true)
			{
			};
		}

		extern "C" void taskReturnedHandler( void )
		{
			// A task's run() function must never return
			setGlobalPointers();
			if( gPtrPort != nullptr )
			{
				gPtrPort->taskExitError(); // does not return
			}
			setInterruptPriorityAndGetOriginal();
			while( true )
			{
			}
		}

		extern "C" void callFirstTime(void)
		{
			setGlobalPointers();
			if( gPtrPort != nullptr)
			{
				TaskData* ptrTaskData = gPtrPort->getCurrentTask();
				if( ptrTaskData != nullptr )
				{
					ITask* ptrTaskInterface = ptrTaskData->getTaskInterfacePtr();
					if( ptrTaskInterface != nullptr )
					{
						return ptrTaskInterface->run();
					}
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
			static_cast<void>(pvParameters);

			/* Simulate the stack frame as it would be created by a context switch interrupt. */

			/* AAPCS: the stack pointer must be 8-byte aligned at a public interface and on exception entry. */
			auto topOfStack = reinterpret_cast<std::uint32_t*>( reinterpret_cast<std::uintptr_t>(pxTopOfStack) & ~static_cast<std::uintptr_t>(7u) );

			/* Hardware-stacked exception frame (8 words, R0 at the lowest address). The frame starts
			   8-byte aligned, so after exception return the task starts with an aligned stack. */
			topOfStack--;
			*topOfStack = 0x01000000u;                                              /* xPSR: Thumb state bit (T bit) */
			topOfStack--;
			/* For strict compliance with the Cortex-M spec, the start address loaded into the PC must have bit 0 clear. */
			*topOfStack = reinterpret_cast<std::uint32_t>( &callFirstTime ) & 0xfffffffeUL;  /* PC */
			topOfStack--;
			*topOfStack = reinterpret_cast<std::uint32_t>( &taskReturnedHandler );   /* LR: reached if run() returns */
			topOfStack -= 5;                                                        /* R12, R3, R2, R1 (not initialised) */
			*topOfStack = 0u;                                                       /* R0: not used */

			/* Software-saved context as restored by PendSV_Handler / SVC_Handler: ldmia {r4-r11, r14} */
			topOfStack--;
			*topOfStack = 0xfffffffdu;                                              /* EXC_RETURN: thread mode, PSP, no FPU frame */
			for( std::uint32_t reg = 0u; reg < 8u; ++reg )
			{
				topOfStack--;
				*topOfStack = 0u;                                                   /* R11 .. R4 */
			}

			return static_cast<void*>(topOfStack);
		}	
	}
}

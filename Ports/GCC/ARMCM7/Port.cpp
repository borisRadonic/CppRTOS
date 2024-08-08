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

		inline void Port::enterCritical()
		{
			nestingCounter++;
			setInterruptPriorityAndGetOriginal(); //disable interrupts
		}

		inline void Port::exitCritical()
		{
			if( nestingCounter != 0 )
			{
				//assert( _nestingCounter != 0)
				//while(true) {};
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
			uint32_t  pri = setInterruptPriorityAndGetOriginal();
			setGlobalPointers();
			gPtrPort->incrementTickCount();
			gPtrPort->tick();
			NVIC_ICSR = ICSR_PENDSVSET_BIT;
			assert( pri != 10000 );
			setInterruptBasePriority( 0 );
		}

		void Port::validateInterruptPriority() const
		{
		}

		void Port::startScheduler()
		{
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
			 /* Simulate the stack frame as it would be created by a context switch interrupt. */

			 auto topOfStack = static_cast<std::uint32_t*>(pxTopOfStack);

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
			 FunctionPointerUnion funcUnion = {};
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
	}
}

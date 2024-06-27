#pragma once

#include <cstdint>
#include <cstddef>

class IPort
{
public:

	
	virtual bool isInsideInterrupt( void ) const = 0;

	virtual void disableInterrupts( void )  const = 0;

	virtual void enableInterrupts( void )  const = 0;

	virtual void enterCritical(void)  const = 0;

	virtual void exitCritical(void)  const = 0;

	virtual void memoryBarrier( void ) const = 0;

	virtual void setupTimerInterrupt( void ) const = 0;

	virtual void validateInterruptPriority(void) const = 0;

	virtual void startScheduler(void)  const = 0;

	virtual void endScheduler(void)  const = 0;

	virtual void startFirstTask(void)  const = 0;

	virtual void enableVFP(void)  const = 0;

	virtual void taskExitError(void)  const = 0;

	virtual void* pxPortInitialiseStack(void* pxTopOfStack, /*TaskFunction_t*/ std::uint32_t pxCode, void* pvParameters) = 0;

	virtual void switchContext(void) const = 0;

};


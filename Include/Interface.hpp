#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

class IPort
{
public:

	
	virtual bool isInsideInterrupt( void ) const = 0;

	virtual void disableInterrupts( void )  const = 0;

	virtual void enableInterrupts( void )  const = 0;

	virtual void enterCritical(void) = 0;

	virtual void exitCritical(void) = 0;

	virtual void memoryBarrier( void ) const = 0;

	virtual void setupTimerInterrupt( void ) const = 0;

	virtual void validateInterruptPriority(void) const = 0;

	virtual void startScheduler(void) = 0;

	virtual void endScheduler(void)  const = 0;

	virtual void startFirstTask(void)  const = 0;

	virtual void enableVFP(void)  const = 0;

	virtual void taskExitError(void) = 0;

	virtual void* initialiseStack(void* pxTopOfStack, std::function<void()> taskFunction, void* pvParameters) = 0;


};


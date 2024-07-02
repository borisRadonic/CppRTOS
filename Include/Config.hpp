#pragma once

#include <cstdint>
#include <cstddef>
#include "stdint.h"



namespace CppRtos
{
	namespace Settings
	{
		//constexpr std::uint32_t MAX_SYSCALL_INTERRUPT_PRIORITY  = 5u;
		constexpr std::size_t MAX_TASKS = 25u;
		constexpr std::uint32_t MAX_PRIORITY  = 56u;
		constexpr std::size_t IDLE_TASK_STACK_SIZE = 1024u;
		constexpr std::uint32_t TICK_RATE_HZ = 1000u;
	}
}


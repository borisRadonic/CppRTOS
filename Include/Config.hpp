#pragma once

#include <cstdint>
#include <cstddef>
#include <assert.h>

namespace CppRtos
{
	namespace Settings
	{
		constexpr std::size_t MAX_TASKS = 32u;
		constexpr std::size_t MAX_TIMERS = 32u;
		static_assert( (MAX_TASKS >= 2u) && (MAX_TASKS <= 32) );
		constexpr std::uint32_t MAX_PRIORITY  = 56u;
		constexpr std::size_t IDLE_TASK_STACK_SIZE = 1024u;
		constexpr std::uint32_t TICK_RATE_HZ = 1000u;
	}
}


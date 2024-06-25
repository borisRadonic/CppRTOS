#pragma once

#include <cstdint>
#include <cstddef>


namespace CppRtos
{
	namespace Settings
	{
		constexpr std::uint32_t MAX_SYSCALL_INTERRUPT_PRIORITY  = 5u;
		constexpr std::size_t MAX_TASKS = 25u;
		constexpr std::uint32_t MAX_PRIORITY  = 56u;
	}
}

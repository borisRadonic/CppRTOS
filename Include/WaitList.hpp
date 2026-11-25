#pragma once

#include <cstdint>

namespace CppRtos
{
	class Kernel;

	/**
	 * @brief Reason why a blocked task was made ready again.
	 */
	enum class WakeReason : std::uint8_t
	{
		eNone     = 0u, /**< Task has not been woken yet (or was not waiting). */
		eSignaled = 1u, /**< The awaited object was signaled (semaphore given, mutex handed over, ...). */
		eTimeout  = 2u  /**< The wait timed out. */
	};

	/**
	 * @brief Set of tasks waiting on a kernel object (semaphore, mutex, event group, queue).
	 *
	 * Stored as a bitmap of task ids (MAX_TASKS <= 32), so it needs no storage per waiter.
	 * Waking selects the waiter with the highest priority; waiters of equal priority are
	 * woken in FIFO order (by the sequence number recorded when they started waiting).
	 *
	 * All operations are performed by the Kernel inside a critical section.
	 */
	class WaitList final
	{
	public:
		WaitList() = default;
		WaitList(const WaitList&) = delete;
		WaitList& operator=(const WaitList&) = delete;
		WaitList(WaitList&&) = delete;
		WaitList& operator=(WaitList&&) = delete;
		~WaitList() = default;

		/**
		 * @brief Returns true if no task is waiting.
		 */
		[[nodiscard]] inline bool isEmpty() const
		{
			return (waitingTasksMask == 0u);
		}

		/**
		 * @brief Returns the bitmap of waiting task ids.
		 */
		[[nodiscard]] inline std::uint32_t getMask() const
		{
			return waitingTasksMask;
		}

	private:
		friend class Kernel;

		std::uint32_t waitingTasksMask = 0u; /**< Bit n set: task with id n is waiting. */
	};
}

/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helper class for calculating remaining time.
 *
 * \since
 * v.5.5.16
 */

#pragma once

#include <chrono>

namespace so_5 {

namespace details {

/*!
 * \brief Helper class for calculating remaining time.
 *
 * Usage example:
 * \code
	so_5::details::remaining_time_counter_t time_counter{ wait_time };
	do
	{
		some_task( time_counter.remaining() );
		time_counter.update();
	}
	while( time_counter );
 * \endcode
 *
 * \since
 * v.5.5.16
 */
class remaining_time_counter_t
	{
		using time_point = std::chrono::steady_clock::time_point;
		using duration = std::chrono::steady_clock::duration;

		/*!
		 * \note This member in not declared as const to allow reassignment
		 * for remaining_time_counter_t object. Like in such cases:
		 * \code
			remaining_time_counter_t counter{ some_initial_value };
			do
				{
					do_something();
					if( needs_restart )
						counter = remaining_time_counter_t{ some_initial_value };
					else
						counter.update();
				}
			while( counter );
		 * \endcode
		 */
		time_point m_last_time;
		duration m_remaining;

	public :
		remaining_time_counter_t( duration remaining )
			:	m_last_time{ std::chrono::steady_clock::now() }
			,	m_remaining{ remaining }
			{}

		void
		update()
			{
				const auto now = std::chrono::steady_clock::now();
				const auto elapsed = now - m_last_time;
				m_last_time = now;

				if( elapsed < m_remaining )
					m_remaining -= elapsed;
				else
					m_remaining = duration::zero();
			}

		duration
		remaining() const { return m_remaining; }

		/*!
		 * \return true if m_remaining is greater than duration::zero().
		 */
		operator bool() const { return duration::zero() < m_remaining; }
	};

} /* namespace details */

} /* namespace so_5 */


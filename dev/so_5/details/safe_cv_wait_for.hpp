/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Helpers for safe calls to condition_variable::wait_for with
 * big timeout intervals.
 *
 * \since
 * v.5.7.2
 */

#pragma once

#include <so_5/details/remaining_time_counter.hpp>

#include <chrono>
#include <condition_variable>

namespace so_5 {

namespace details {

/*!
 * \brief Helper function for safe call of condition_variable::wait_for
 * with possible big timeouts.
 *
 * In some places of SObjectizer's code big values of wait-time can
 * be used for std::condition_variable::wait_for. For example,
 * that values can be produced by so_5::infinite_wait_special_timevalue()
 * function.
 *
 * If such big value will be passed to std::condition_variable::wait_for
 * the wait_for can return immediately without any waiting.
 *
 * To avoid that behavior this helper function should be used instead
 * of the direct call to condition_variable::wait_for.
 *
 * \since
 * v.5.7.2
 */
template< typename Predicate >
bool
wait_for_big_interval(
	std::unique_lock< std::mutex > & lock,
	std::condition_variable & cv,
	std::chrono::steady_clock::duration timeout,
	Predicate pred )
	{
		constexpr std::chrono::hours border{24};
		if( timeout <= border )
			{
				return cv.wait_for( lock, timeout, pred );
			}
		else
			{
				remaining_time_counter_t counter{ timeout };
				while( counter )
					{
						const auto r = counter.remaining();
						const auto wait_time = (r <= border ? r : border);

						const auto wait_result =
								cv.wait_for( lock, wait_time, pred );

						if( wait_result )
							return wait_result;
						else
							counter.update();
					}
			}

		return false;
	}

} /* namespace details */

} /* namespace so_5 */


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

#if !defined( SO_5_DETAILS_SAFE_CV_WAIT_FOR_HPP )
#define SO_5_DETAILS_SAFE_CV_WAIT_FOR_HPP

#include <chrono>

#include <so_5/details/remaining_time_counter.hpp>

namespace so_5 {

namespace details {

namespace wait_for_big_interval_impl
{

/*!
 * @brief Actual implementation of wait_for_big_interval.
 *
 * @note
 * It's expected that Predicate_Handler will be no_predicate_handler_t
 * or actual_predicate_handler_t.
 *
 * @since v.5.8.7
 */
template< typename Predicate_Handler >
bool
do_wait(
	std::unique_lock< std::mutex > & lock,
	std::condition_variable & cv,
	std::chrono::steady_clock::duration timeout,
	Predicate_Handler & handler )
	{
		constexpr std::chrono::hours border{24};
		if( timeout <= border )
			{
				return handler(lock, cv, timeout );
			}
		else
			{
				remaining_time_counter_t counter{ timeout };
				while( counter )
					{
						const auto r = counter.remaining();
						const auto wait_time = (r <= border ? r : border);

						const auto wait_result = handler( lock, cv, wait_time );

						if( wait_result )
							return wait_result;
						else
							counter.update();
					}
			}

		return false;
	}

//
// no_predicate_handler_t
//
/*!
 * @brief Helper type for do_wait().
 *
 * Implements the case when there is no predicate for
 * std::condition_variable::wait_for.
 *
 * @since v.5.8.7
 */
struct no_predicate_handler_t
	{
		[[nodiscard]]
		bool
		operator()(
			std::unique_lock< std::mutex > & lock,
			std::condition_variable & cv,
			std::chrono::steady_clock::duration timeout )
			{
				const auto r = cv.wait_for( lock, timeout );
				return std::cv_status::no_timeout == r;
			}
	};

//
// actual_predicate_handler_t
//
/*!
 * @brief Helper type for do_wait().
 *
 * Implements the case when there is an actual predicate for
 * std::condition_variable::wait_for.
 *
 * @since v.5.8.7
 */
template< typename Predicate >
struct actual_predicate_handler_t
	{
		Predicate & m_predicate;

		[[nodiscard]]
		bool
		operator()(
			std::unique_lock< std::mutex > & lock,
			std::condition_variable & cv,
			std::chrono::steady_clock::duration timeout )
			{
				return cv.wait_for( lock, timeout, m_predicate );
			}
	};

} /* wait_for_big_interval_impl */

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
 * \since v.5.7.2
 */
template< typename Predicate >
bool
wait_for_big_interval(
	std::unique_lock< std::mutex > & lock,
	std::condition_variable & cv,
	std::chrono::steady_clock::duration timeout,
	Predicate pred )
	{
		using namespace wait_for_big_interval_impl;

		actual_predicate_handler_t< Predicate > handler{ .m_predicate = pred };
		return do_wait( lock, cv, timeout, handler );
	}

/*!
 * \brief Helper function for safe call of condition_variable::wait_for
 * with possible big timeouts.
 *
 * @note
 * This version doesn't receive a predicate for
 * std::condition_variable::wait_for.
 *
 * @brief v.5.8.7
 */
inline
bool
wait_for_big_interval(
	std::unique_lock< std::mutex > & lock,
	std::condition_variable & cv,
	std::chrono::steady_clock::duration timeout )
	{
		using namespace wait_for_big_interval_impl;

		no_predicate_handler_t handler;
		return do_wait( lock, cv, timeout, handler );
	}

} /* namespace details */

} /* namespace so_5 */

#endif


/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.18
 *
 * \brief Data types for work thread's activity stats.
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <iostream>
#include <type_traits>

namespace so_5
{

namespace stats
{

/*!
 * \brief An alias for clock type to be used for statistics.
 *
 * \since
 * v.5.5.18
 */
using clock_type_t = std::conditional<
				std::chrono::high_resolution_clock::is_steady,
				std::chrono::high_resolution_clock,
				std::chrono::steady_clock
		>::type;

static_assert( clock_type_t::is_steady,
		"clock_type_t must be steady clock!" );

/*!
 * \brief An alias for type for representing time duration.
 *
 * \since
 * \v.5.5.18
 */
using duration_t = clock_type_t::duration;

/*!
 * \brief Statistics of some activity.
 *
 * \since
 * v.5.5.18
 */
struct activity_stats_t
	{
		//! Count of events in that period of time.
		std::uint_fast64_t m_count{};

		//! Total time spent for events in that period of time.
		duration_t m_total_time{};

		//! Average time for one event.
		duration_t m_avg_time{};
	};

/*!
 * \brief Helper for printing value of activity_stats.
 *
 * \since
 * v.5.5.18
 */
inline std::ostream &
operator<<( std::ostream & to, const activity_stats_t & what )
{
	auto to_ms = []( const duration_t & d ) {
		return std::chrono::duration_cast< std::chrono::nanoseconds >( d )
				.count() / 1000000.0;
	};

	to << "[count=" << what.m_count
		<< ";total=" << to_ms(what.m_total_time)
		<< "ms;avg=" << to_ms(what.m_avg_time) << "ms]";

	return to;
}

/*!
 * \brief Stats for a work thread activity.
 *
 * \since
 * v.5.5.18
 */
struct work_thread_activity_stats_t
	{
		//! Stats for processed events.
		activity_stats_t m_working_stats{};

		//! Stats for waiting periods.
		activity_stats_t m_waiting_stats{};
	};

namespace details
{

/*!
 * \brief A function for calculating average value.
 *
 * Uses modified moving average (also known as running moving average)
 * which is calculated as: MMA(t) = (last + (N-1)*MMA(t-1))/N.
 *
 * \since
 * v.5.5.18
 */
inline duration_t
calc_avg_time(
	//! Total count of samples.
	std::uint_fast64_t count,
	//! Previous average value.
	duration_t previous,
	//! Duration of last activity (or waiting).
	duration_t last )
{
	const int N = count > 100u ? 100 : (count ? static_cast< int >(count) : 1);
	return (last + (N-1) * previous) / N;
}

/*!
 * \brief Helper function for simplification of current stats update.
 *
 * \since
 * v.5.5.18
 */
inline void
update_stats_from_duration(
	activity_stats_t & value_to_update,
	clock_type_t::duration last_duration )
{
	value_to_update.m_total_time += last_duration;
	value_to_update.m_avg_time = calc_avg_time(
			value_to_update.m_count,
			value_to_update.m_avg_time, last_duration );
}

/*!
 * \brief Helper function for simplification of current stats update.
 *
 * \since
 * v.5.5.18
 */
inline void
update_stats_from_current_time(
	activity_stats_t & value_to_update,
	clock_type_t::time_point activity_started_at )
{
	update_stats_from_duration(
			value_to_update,
			clock_type_t::now() - activity_started_at );
}

} /* namespace details */

} /* namespace stats */

} /* namespace so_5 */


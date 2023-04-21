/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief A data source class for run-time monitoring of timer_thread.
 */

#include <so_5/stats/impl/ds_timer_thread_stats.hpp>

#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_timer_thread_stats_t
//
ds_timer_thread_stats_t::ds_timer_thread_stats_t(
	so_5::environment_infrastructure_t & what )
	:	m_what( what )
	{}

void
ds_timer_thread_stats_t::distribute(
	const mbox_t & distribution_mbox )
	{
		const auto stats = m_what.query_timer_thread_stats();

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::timer_thread(),
				suffixes::timer_single_shot_count(),
				stats.m_single_shot_count );

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::timer_thread(),
				suffixes::timer_periodic_count(),
				stats.m_periodic_count );
	}

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


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

#include <so_5/rt/stats/impl/h/ds_timer_thread_stats.hpp>

#include <so_5/rt/stats/h/messages.hpp>
#include <so_5/rt/stats/h/std_names.hpp>

#include <so_5/rt/h/send_functions.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_timer_thread_stats_t
//
ds_timer_thread_stats_t::ds_timer_thread_stats_t(
	outliving_reference_t< repository_t > repo,
	so_5::environment_infrastructure_t & what )
	:	auto_registered_source_t( std::move(repo) )
	,	m_what( what )
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


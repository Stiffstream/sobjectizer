/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief A data source class for run-time monitoring of agent_core.
 */

#include <so_5/stats/impl/h/ds_agent_core_stats.hpp>

#include <so_5/stats/h/messages.hpp>
#include <so_5/stats/h/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_agent_core_stats_t
//
ds_agent_core_stats_t::ds_agent_core_stats_t(
	outliving_reference_t< repository_t > repo,
	so_5::environment_infrastructure_t & what )
	:	auto_registered_source_t( std::move(repo) )
	,	m_what( what )
	{}

void
ds_agent_core_stats_t::distribute(
	const mbox_t & distribution_mbox )
	{
		auto stats = m_what.query_coop_repository_stats();

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::coop_repository(),
				suffixes::coop_reg_count(),
				stats.m_registered_coop_count );

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::coop_repository(),
				suffixes::coop_dereg_count(),
				stats.m_deregistered_coop_count );

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::coop_repository(),
				suffixes::agent_count(),
				stats.m_total_agent_count );

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::coop_repository(),
				suffixes::coop_final_dereg_count(),
				stats.m_final_dereg_coop_count );
	}

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


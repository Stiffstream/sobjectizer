/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief A data source class for run-time monitoring of mbox_core.
 */

#include <so_5/stats/impl/h/ds_mbox_core_stats.hpp>

#include <so_5/stats/h/messages.hpp>
#include <so_5/stats/h/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_mbox_core_stats_t
//
ds_mbox_core_stats_t::ds_mbox_core_stats_t(
	outliving_reference_t< repository_t > repo,
	so_5::impl::mbox_core_t & what )
	:	auto_registered_source_t( std::move(repo) )
	,	m_what( what )
	{}

void
ds_mbox_core_stats_t::distribute(
	const mbox_t & distribution_mbox )
	{
		auto stats = m_what.query_stats();

		send< messages::quantity< std::size_t > >( distribution_mbox,
				prefixes::mbox_repository(),
				suffixes::named_mbox_count(),
				stats.m_named_mbox_count );
	}

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


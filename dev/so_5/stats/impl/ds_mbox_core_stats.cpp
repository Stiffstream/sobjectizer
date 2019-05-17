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

#include <so_5/stats/impl/ds_mbox_core_stats.hpp>

#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_mbox_core_stats_t
//
ds_mbox_core_stats_t::ds_mbox_core_stats_t(
	so_5::impl::mbox_core_t & what )
	:	m_what( what )
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


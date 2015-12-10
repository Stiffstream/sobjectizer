/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief A data source class for run-time monitoring of agent_core.
 */

#pragma once

#include <so_5/rt/stats/h/repository.hpp>

#include <so_5/rt/impl/h/agent_core.hpp>

namespace so_5 {

namespace stats {

namespace impl {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// ds_agent_core_stats_t
//
/*!
 * \since v.5.5.4
 * \brief A data source for distributing information about agent_core.
 */
class ds_agent_core_stats_t : public auto_registered_source_t
	{
	public :
		ds_agent_core_stats_t(
			//! Repository for data source.
			repository_t & repo,
			//! What to watch.
			//! This reference must stay valid during all lifetime of
			//! the data source object.
			so_5::impl::agent_core_t & what );

		virtual void
		distribute(
			const mbox_t & distribution_mbox ) override;

	private :
		so_5::impl::agent_core_t & m_what;
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


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

#pragma once

#include <so_5/stats/repository.hpp>

#include <so_5/environment_infrastructure.hpp>

namespace so_5 {

namespace stats {

namespace impl {

//
// ds_agent_core_stats_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A data source for distributing information about agent_core.
 */
class ds_agent_core_stats_t final : public source_t
	{
	public :
		ds_agent_core_stats_t(
			//! What to watch.
			//! This reference must stay valid during all lifetime of
			//! the data source object.
			so_5::environment_infrastructure_t & what );

		void
		distribute(
			const mbox_t & distribution_mbox ) override;

	private :
		so_5::environment_infrastructure_t & m_what;
	};

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


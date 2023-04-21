/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.6.0
 *
 * \file
 * \brief Helper class for accessing private functionality of agent-class.
 */

#pragma once

#include <so_5/agent.hpp>

namespace so_5 {

namespace impl {

//
// internal_agent_iface_t
//
/*!
 * \brief A special class for access to private/protected members
 * of agent class.
 *
 * \since
 * v.5.6.0
 */
class internal_agent_iface_t final
	{
		agent_t & m_agent;

	public:
		explicit internal_agent_iface_t( agent_t & agent ) noexcept
			:	m_agent{ agent }
			{}

		void
		bind_to_coop( coop_t & coop )
			{
				m_agent.bind_to_coop( coop );
			}

		void
		initiate_agent_definition()
			{
				m_agent.so_initiate_agent_definition();
			}

		void
		shutdown_agent() noexcept
			{
				m_agent.shutdown_agent();
			}
	};

} /* namespace impl */

} /* so_5 */


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

#include <so_5/ret_code.hpp>

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

		/*!
		 * \brief Setter for disp_binder.
		 *
		 * \attention
		 * It's not a thread safe method. But it's expected that it will be used
		 * in the context where just one entity owns the pointer to the agent.
		 *
		 * \since v.5.7.5
		 */
		void
		set_disp_binder( disp_binder_shptr_t binder )
			{
				if( m_agent.m_disp_binder )
					SO_5_THROW_EXCEPTION(
							rc_disp_binder_already_set_for_agent,
							"m_agent.m_disp_binder is not nullptr when "
							"set_disp_binder is called" );

				m_agent.m_disp_binder = std::move(binder);
			}

		/*!
		 * \brief Getter for disp_binder.
		 *
		 * \attention
		 * It's not a thread safe method. But it's expected that it will be used
		 * in the context where just one entity owns the pointer to the agent.
		 *
		 * \since v.5.7.5
		 */
		[[nodiscard]]
		disp_binder_t &
		query_disp_binder() const
			{
				if( !m_agent.m_disp_binder )
					SO_5_THROW_EXCEPTION(
							rc_no_disp_binder_for_agent,
							"m_agent.m_disp_binder is nullptr when "
							"query_disp_binder is called" );

				return *m_agent.m_disp_binder;
			}

		/*!
		 * \brief Helper method that drops pointer to disp_binder.
		 *
		 * This method is intended to be used for rollback actions
		 * (for example, when disp_binder is set for the agent, but
		 * agent can't be stored in a coop).
		 *
		 * \attention
		 * It's not a thread safe method. But it's expected that it will be used
		 * in the context where just one entity owns the pointer to the agent.
		 *
		 * \since v.5.7.5
		 */
		void
		drop_disp_binder() noexcept
			{
				m_agent.m_disp_binder.reset();
			}
	};

} /* namespace impl */

} /* so_5 */


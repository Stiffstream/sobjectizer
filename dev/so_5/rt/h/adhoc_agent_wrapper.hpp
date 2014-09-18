/*
	SObjectizer 5.
*/

/*!
 * \file
 * \since v.5.3.0
 * \brief Special agent class which would be used as wrapper for ad-hoc agents.
 */

#if !defined( SO_5__RT__ADHOC_AGENT_WRAPPER_HPP )
#define SO_5__RT__ADHOC_AGENT_WRAPPER_HPP

#include <so_5/rt/h/agent.hpp>

#include <functional>

namespace so_5
{

namespace rt
{

/*!
 * \since v.5.3.0
 * \brief Special agent class which would be used as wrapper for ad-hoc agents.
 *
 * \note The presence and usefulness of this class is based on the fact
 * that it is not necessary to subscribe agent exactly in so_define_agent()
 * method. The current implementation of agents and SObjectizer Run-Time
 * allows to subscribe and unsubscribe agent in any place and at any time.
 */
class SO_5_TYPE adhoc_agent_wrapper_t
	:	public agent_t
	{
	public :
		adhoc_agent_wrapper_t( so_environment_t & env );
		virtual ~adhoc_agent_wrapper_t();

		//! Set function for reaction on work start.
		void
		set_on_evt_start( std::function< void() > handler );

		//! Set function for reaction on work finish.
		void
		set_on_evt_finish( std::function< void() > handler );

		//! Set reaction for non-handled exceptions.
		void
		set_exception_reaction(
			exception_reaction_t reaction );

		virtual void
		so_evt_start();

		virtual void
		so_evt_finish();

		virtual exception_reaction_t
		so_exception_reaction() const;

	private :
		std::function< void() > m_on_start;
		std::function< void() > m_on_finish;
		exception_reaction_t m_exception_reaction;
	};

//
// adhoc_agent_definition_proxy_t
//
/*!
 * \since v.5.3.0
 * \brief A proxy for process of ad-hoc agent definition.
 */
class adhoc_agent_definition_proxy_t
	{
	public :
		adhoc_agent_definition_proxy_t(
			adhoc_agent_wrapper_t * agent )
			:	m_agent( *agent )
			{}

		/*!
		 * Subscription of event- or service-handler to message.
		 */
		template< class LAMBDA >
		inline adhoc_agent_definition_proxy_t &
		event(
			const mbox_ref_t & mbox,
			LAMBDA lambda,
			thread_safety_t thread_safety = not_thread_safe )
			{
				m_agent.so_subscribe( mbox ).event( lambda, thread_safety );

				return *this;
			}

		/*!
		 * Subscription of event- or service-handler to signal.
		 */
		template< class MESSAGE, class LAMBDA >
		inline adhoc_agent_definition_proxy_t &
		event(
			const mbox_ref_t & mbox,
			signal_indicator_t< MESSAGE > (*indicator)(),
			LAMBDA lambda,
			thread_safety_t thread_safety = not_thread_safe )
			{
				m_agent.so_subscribe( mbox )
						.event( indicator, lambda, thread_safety );

				return *this;
			}

		/*!
		 * Define reaction to work start.
		 */
		inline adhoc_agent_definition_proxy_t
		on_start( std::function< void() > handler )
			{
				m_agent.set_on_evt_start( handler );

				return *this;
			}

		/*!
		 * Define reaction to work finish.
		 */
		inline adhoc_agent_definition_proxy_t
		on_finish( std::function< void() > handler )
			{
				m_agent.set_on_evt_finish( handler );

				return *this;
			}

		/*!
		 * Define reaction to non-handled exception.
		 */
		inline adhoc_agent_definition_proxy_t
		exception_reaction(
			exception_reaction_t reaction )
			{
				m_agent.set_exception_reaction( reaction );
			}

	private :
		adhoc_agent_wrapper_t & m_agent;
	};

} /* namespace rt */

} /* namespace so_5 */

#endif

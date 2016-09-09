/*
	SObjectizer 5.
*/

/*!
 * \file
 * \since
 * v.5.3.0
 *
 * \brief Special agent class which would be used as wrapper for ad-hoc agents.
 */

#pragma once

#include <so_5/rt/h/agent.hpp>

#include <functional>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

/*!
 * \since
 * v.5.3.0
 *
 * \brief Special agent class which would be used as wrapper for ad-hoc agents.
 *
 * \note The presence and usefulness of this class is based on the fact
 * that it is not necessary to subscribe agent exactly in so_define_agent()
 * method. The current implementation of agents and SObjectizer Run-Time
 * allows to subscribe and unsubscribe agent in any place and at any time.
 */
class SO_5_TYPE adhoc_agent_wrapper_t : public agent_t
	{
	public :
		adhoc_agent_wrapper_t( agent_context_t ctx );
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
 * \since
 * v.5.3.0
 *
 * \brief A proxy for process of ad-hoc agent definition.
 */
class adhoc_agent_definition_proxy_t
	{
	public :
		adhoc_agent_definition_proxy_t(
			adhoc_agent_wrapper_t * agent )
			:	m_agent( agent )
			{}

		/*!
		 * Subscription of event- or service-handler to message.
		 */
		template< class LAMBDA >
		inline adhoc_agent_definition_proxy_t &
		event(
			const mbox_t & mbox,
			LAMBDA lambda,
			thread_safety_t thread_safety = not_thread_safe )
			{
				m_agent->so_subscribe( mbox ).event( lambda, thread_safety );

				return *this;
			}

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Subscription of event- or service-handler to message.
		 *
		 * \note Uses direct mbox of the ad-hoc agent.
		 *
		 * \par Usage sample:
		 * \code
		   so_5::coop_unique_ptr_t coop = env.create_coop(...);
		   auto a = coop->define_agent();
			a.event< msg_my_signal >( a, [=] { ... } );
		 * \endcode
		 */
		template< class LAMBDA >
		inline adhoc_agent_definition_proxy_t &
		event(
			const adhoc_agent_definition_proxy_t & self,
			LAMBDA lambda,
			thread_safety_t thread_safety = not_thread_safe )
			{
				return this->event( self.direct_mbox(), lambda, thread_safety );
			}

		/*!
		 * Subscription of event- or service-handler to signal.
		 */
		template< class MESSAGE, class LAMBDA >
		inline adhoc_agent_definition_proxy_t &
		event(
			const mbox_t & mbox,
			signal_indicator_t< MESSAGE > (*indicator)(),
			LAMBDA lambda,
			thread_safety_t thread_safety = not_thread_safe )
			{
				m_agent->so_subscribe( mbox )
						.event( indicator, lambda, thread_safety );

				return *this;
			}

		/*!
		 * \since
		 * v.5.5.1
		 *
		 * \brief Subscription of event- or service-handler to signal.
		 *
		 * \par Usage sample:
		 * \code
		   so_5::coop_unique_ptr_t coop = env.create_coop(...);
		   coop->define_agent()
		   	.event< msg_my_signal >( mbox, [=] { ... } );
		 * \endcode
		 */
		template< class SIGNAL, typename... ARGS >
		inline adhoc_agent_definition_proxy_t &
		event(
			const mbox_t & mbox,
			ARGS&&... args )
			{
				return this->event( mbox, signal< SIGNAL >,
						std::forward< ARGS >(args)... );
			}

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Subscription of event- or service-handler to signal.
		 *
		 * \note Uses direct mbox of the ad-hoc agent.
		 *
		 * \par Usage sample:
		 * \code
		   so_5::coop_unique_ptr_t coop = env.create_coop(...);
		   auto a = coop->define_agent();
			a.event< msg_my_signal >( a, [=] { ... } );
		 * \endcode
		 */
		template< class SIGNAL, typename... ARGS >
		inline adhoc_agent_definition_proxy_t &
		event(
			const adhoc_agent_definition_proxy_t & self,
			ARGS&&... args )
			{
				return this->event( self.direct_mbox(),
						signal< SIGNAL >,
						std::forward< ARGS >(args)... );
			}

		/*!
		 * Define reaction to work start.
		 */
		inline adhoc_agent_definition_proxy_t &
		on_start( std::function< void() > handler )
			{
				m_agent->set_on_evt_start( handler );

				return *this;
			}

		/*!
		 * Define reaction to work finish.
		 */
		inline adhoc_agent_definition_proxy_t &
		on_finish( std::function< void() > handler )
			{
				m_agent->set_on_evt_finish( handler );

				return *this;
			}

		/*!
		 * Define reaction to non-handled exception.
		 */
		inline adhoc_agent_definition_proxy_t &
		exception_reaction(
			exception_reaction_t reaction )
			{
				m_agent->set_exception_reaction( reaction );

				return *this;
			}

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Access to agent's direct mbox.
		 *
		 * \par Usage sample:
		 \code
		 auto coop = env.create_coop();
		 auto adhoc_agent = coop.define_agent();
		 adhoc_agent.event( adhoc_agent.direct_mbox(), []{ ... } );
		 \endcode
		 */
		inline const mbox_t &
		direct_mbox() const
			{
				return m_agent->so_direct_mbox();
			}

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief Access to agent's environment.
		 */
		inline environment_t &
		environment() const
			{
				return m_agent->so_environment();
			}

	private :
		adhoc_agent_wrapper_t * m_agent;
	};

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::adhoc_agent_wrapper_t
 * instead.
 */
using adhoc_agent_wrapper_t = so_5::adhoc_agent_wrapper_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::adhoc_agent_definition_proxy_t instead.
 */
using adhoc_agent_definition_proxy_t = so_5::adhoc_agent_definition_proxy_t;

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif


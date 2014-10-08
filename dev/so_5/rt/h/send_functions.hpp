/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since v.5.5.1
 * \brief Implementation of free functions send/send_delayed.
 */

#pragma once

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/environment.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

	/*
	 * This is helpers for so_5::send implementation.
	 */

	template< class MESSAGE, bool IS_SIGNAL >
	struct instantiator_and_sender_t
		{
			void
			send( const so_5::rt::mbox_ref_t & to )
				{
					std::unique_ptr< MESSAGE > msg( new MESSAGE() );

					to->deliver_message( std::move( msg ) );
				}

			void
			send_delayed(
				so_5::rt::environment_t & env,
				const so_5::rt::mbox_ref_t & to,
				std::chrono::steady_clock::duration pause )
				{
					std::unique_ptr< MESSAGE > msg( new MESSAGE() );

					env.single_timer( std::move( msg ), to, pause );
				}
		};

	template< class MESSAGE >
	struct instantiator_and_sender_t< MESSAGE, true >
		{
			void
			send( const so_5::rt::mbox_ref_t & to )
				{
					to->deliver_signal< MESSAGE >();
				}

			void
			send_delayed(
				so_5::rt::environment_t & env,
				const so_5::rt::mbox_ref_t & to,
				std::chrono::steady_clock::duration pause )
				{
					env.single_timer< MESSAGE >( to, pause );
				}
		};

} /* namespace impl */

} /* namespace rt */

/*!
 * \since v.5.5.1
 * \brief A utility function for creating and delivering a message.
 */
template< typename MESSAGE, typename... ARGS >
void
send( const so_5::rt::mbox_ref_t & to, ARGS&&... args )
	{
		std::unique_ptr< MESSAGE > msg(
				new MESSAGE( std::forward<ARGS>(args)... ) );

		to->deliver_message( std::move( msg ) );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for sending a signal.
 */
template< typename MESSAGE >
void
send( const so_5::rt::mbox_ref_t & to )
	{
		so_5::rt::impl::instantiator_and_sender_t<
				MESSAGE,
				std::is_base_of< so_5::rt::signal_t, MESSAGE >::value > helper;

		helper.send( to );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for creating and delivering a delayed message.
 */
template< typename MESSAGE, typename... ARGS >
void
send_delayed(
	//! An environment to be used for timer.
	so_5::rt::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::rt::mbox_ref_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	ARGS&&... args )
	{
		std::unique_ptr< MESSAGE > msg(
				new MESSAGE( std::forward<ARGS>(args)... ) );

		env.single_timer( std::move( msg ), to, pause );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for creating and delivering a delayed message.
 *
 * Gets the Environment from the agent specified.
 */
template< typename MESSAGE, typename... ARGS >
void
send_delayed(
	//! An agent whos environment must be used.
	so_5::rt::agent_t & agent,
	//! Mbox for the message to be sent to.
	const so_5::rt::mbox_ref_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	ARGS&&... args )
	{
		send_delayed< MESSAGE >( agent.so_environment(), to, pause,
				std::forward< ARGS >(args)... );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for creating and delivering a delayed message
 * to the agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 */
template< typename MESSAGE, typename... ARGS >
void
send_delayed_to_agent(
	//! An agent whos environment must be used.
	so_5::rt::agent_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	ARGS&&... args )
	{
		send_delayed< MESSAGE >(
				agent.so_environment(),
				agent.so_direct_mbox(),
				pause,
				std::forward< ARGS >(args)... );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for delivering a delayed signal.
 */
template< typename MESSAGE >
void
send_delayed(
	//! An environment to be used for timer.
	so_5::rt::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::rt::mbox_ref_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause )
	{
		so_5::rt::impl::instantiator_and_sender_t<
				MESSAGE,
				std::is_base_of< so_5::rt::signal_t, MESSAGE >::value > helper;

		helper.send_delayed( env, to, pause );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for delivering a delayed signal.
 *
 * Gets the Environment from the agent specified.
 */
template< typename MESSAGE >
void
send_delayed(
	//! An agent whos environment must be used.
	so_5::rt::agent_t & agent,
	//! Mbox for the message to be sent to.
	const so_5::rt::mbox_ref_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause )
	{
		send_delayed< MESSAGE >( agent.so_environment(), to, pause );
	}

/*!
 * \since v.5.5.1
 * \brief A utility function for delivering a delayed signal to
 * the agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 */
template< typename MESSAGE >
void
send_delayed_to_agent(
	//! An agent whos environment must be used.
	so_5::rt::agent_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause )
	{
		send_delayed< MESSAGE >(
				agent.so_environment(),
				agent.so_direct_mbox(),
				pause );
	}

} /* namespace so_5 */


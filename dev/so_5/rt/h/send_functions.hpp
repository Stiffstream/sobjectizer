/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.1
 *
 * \brief Implementation of free functions send/send_delayed.
 */

#pragma once

#include <so_5/rt/h/environment.hpp>

#include <so_5/h/compiler_features.hpp>

namespace so_5
{

namespace impl
{

	/*
	 * This is helpers for so_5::send implementation.
	 */

	template< class Message, bool Is_Signal >
	struct instantiator_and_sender_base
		{
			template< typename... Args >
			static void
			send(
				const so_5::mbox_t & to,
				Args &&... args )
				{
					to->deliver_message(
						message_payload_type< Message >::subscription_type_index(),
						so_5::details::make_message_instance< Message >(
								std::forward< Args >( args )...),
						message_payload_type< Message >::mutability() );
				}

			template< typename... Args >
			static void
			send_delayed(
				so_5::environment_t & env,
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				Args &&... args )
				{
					env.single_timer(
							message_payload_type< Message >::subscription_type_index(),
							so_5::details::make_message_instance< Message >(
									std::forward<Args>(args)...),
							message_payload_type< Message >::mutability(),
							to,
							pause );
				}

			template< typename... Args >
			SO_5_NODISCARD static timer_id_t
			send_periodic(
				so_5::environment_t & env,
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				std::chrono::steady_clock::duration period,
				Args &&... args )
				{
					return env.schedule_timer( 
							message_payload_type< Message >::subscription_type_index(),
							so_5::details::make_message_instance< Message >(
								std::forward< Args >( args )...),
							message_payload_type< Message >::mutability(),
							to,
							pause,
							period );
				}
		};

	template< class Message >
	struct instantiator_and_sender_base< Message, true >
		{
			//! Type of signal to be delivered.
			using actual_signal_type = typename message_payload_type< Message >::subscription_type;

			static void
			send( const so_5::mbox_t & to )
				{
					to->deliver_signal< actual_signal_type >();
				}

			static void
			send_delayed(
				so_5::environment_t & env,
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause )
				{
					env.single_timer< actual_signal_type >(
							message_payload_type<Message>::subscription_type_index(),
							to,
							pause );
				}

			SO_5_NODISCARD static timer_id_t
			send_periodic(
				so_5::environment_t & env,
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				std::chrono::steady_clock::duration period )
				{
					return env.schedule_timer< actual_signal_type >( to, pause, period );
				}
		};

	template< class Message >
	struct instantiator_and_sender
		:	public instantiator_and_sender_base<
				Message,
				is_signal< typename message_payload_type< Message >::payload_type >::value >
		{};

} /* namespace impl */

/*!
 * \since
 * v.5.5.9
 *
 * \brief Implementation details for send-family and request_future/value helper functions.
 */
namespace send_functions_details {

inline const so_5::mbox_t &
arg_to_mbox( const so_5::mbox_t & mbox ) { return mbox; }

inline const so_5::mbox_t &
arg_to_mbox( const so_5::agent_t & agent ) { return agent.so_direct_mbox(); }

inline const so_5::mbox_t &
arg_to_mbox( const so_5::adhoc_agent_definition_proxy_t & agent ) { return agent.direct_mbox(); }

inline so_5::mbox_t
arg_to_mbox( const so_5::mchain_t & chain ) { return chain->as_mbox(); }

inline so_5::environment_t &
arg_to_env( const so_5::agent_t & agent ) { return agent.so_environment(); }

inline so_5::environment_t &
arg_to_env( const so_5::adhoc_agent_definition_proxy_t & agent ) { return agent.environment(); }

inline so_5::environment_t &
arg_to_env( const so_5::mchain_t & chain ) { return chain->environment(); }

} /* namespace send_functions_details */

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a message or a signal.
 *
 * \note Since v.5.5.9 can accept const references to so_5::mbox_t,
 * so_5::agent_t and so_5::adhoc_agent_definition_proxy_t.
 *
 * \note Since v.5.5.13 can send also a signal.
 *
 * \tparam Message type of message to be sent.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t or
 * so_5::adhoc_agent_definition_proxy_t (in two later cases agent's direct
 * mbox will be used).
 * \tparam Args arguments for Message's constructor.
 *
 * \par Usage samples:
 * \code
	struct hello_msg { std::string greeting; std::string who };

	// Send to mbox.
	so_5::send< hello_msg >( env.create_mbox( "hello" ), "Hello", "World!" );

	// Send to agent.
	class demo_agent : public so_5::agent_t
	{
	public :
		...
		virtual void so_evt_start() override
		{
			...
			so_5::send< hello_msg >( *this, "Hello", "World!" );
		}
	};

	// Send to ad-hoc agent.
	env.introduce_coop( []( so_5::coop_t & coop ) {
		auto a = coop.define_agent();
		a.on_start( [a] {
			...
			so_5::send< hello_msg >( a, "Hello", "World!" );
		} );
		...
	} );

	struct turn_on : public so_5::signal_t {};

	// Send to mbox.
	so_5::send< turn_on >( env.create_mbox( "engine" ) );

	// Send to agent.
	class engine_agent : public so_5::agent_t
	{
	public :
		...
		virtual void so_evt_start() override
		{
			...
			so_5::send< turn_on >( *this );
		}
	};

	// Send to ad-hoc agent.
	env.introduce_coop( []( so_5::coop_t & coop ) {
		auto a = coop.define_agent();
		a.on_start( [a] {
			...
			so_5::send< turn_on >( a );
		} );
		...
	} );
 * \endcode
 */
template< typename Message, typename Target, typename... Args >
void
send( Target && to, Args&&... args )
	{
		so_5::impl::instantiator_and_sender< Message >::send(
				send_functions_details::arg_to_mbox( std::forward<Target>(to) ),
				std::forward<Args>(args)... );
	}

/*!
 * \brief A version of %send function for redirection of a message
 * from exising message hood.
 *
 * \tparam Message a type of message to be redirected (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			so_5::send(another_mbox, cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			so_5::send(another_mbox, std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
typename std::enable_if< !is_signal< Message >::value >::type
send( Target && to, mhood_t< Message > what )
	{
		send_functions_details::arg_to_mbox( std::forward<Target>(to) )->
				deliver_message(
						message_payload_type<Message>::subscription_type_index(),
						what.make_reference() );
	}

/*!
 * \brief A version of %send function for redirection of a signal
 * from exising message hood.
 *
 * \tparam Message a type of signal to be redirected (it can be
 * in form of Sig or so_5::immutable_msg<Sig>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_signal(mhood_t<some_signal> cmd) {
			so_5::send(another_mbox, cmd);
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
typename std::enable_if< is_signal< Message >::value >::type
send( Target && to, mhood_t< Message > /*what*/ )
	{
		send_functions_details::arg_to_mbox( std::forward<Target>(to) )->
				template deliver_signal<
						typename message_payload_type<Message>::subscription_type >();
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a message to
 * the agent's direct mbox.
 */
template< typename Message, typename... Args >
void
send_to_agent( const so_5::agent_t & receiver, Args&&... args )
	{
		send< Message >( receiver, std::forward<Args>(args)... );
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief A utility function for creating and delivering a message to
 * the ad-hoc agent's direct mbox.
 */
template< typename Message, typename... Args >
void
send_to_agent(
	const so_5::adhoc_agent_definition_proxy_t & receiver,
	Args&&... args )
	{
		send< Message >( receiver, std::forward<Args>(args)... );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \brief A utility function for creating and delivering a delayed message.
 */
template< typename Message, typename... Args >
void
send_delayed(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	Args&&... args )
	{
		so_5::impl::instantiator_and_sender< Message >::send_delayed(
				env, to, pause, std::forward<Args>(args)... );
	}

/*!
 * \brief A utility function for creating and delivering a delayed message
 * to the specified destination.
 *
 * Agent, ad-hoc agent or mchain can be used as \a target.
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \tparam Message type of message or signal to be sent.
 * \tparam Target can be so_5::agent_t, so_5::adhoc_agent_definition_proxy_t or
 * so_5::mchain_t.
 * \tparam Args list of arguments for Message's constructor.
 *
 * \since
 * v.5.5.19
 */
template< typename Message, typename Target, typename... Args >
void
send_delayed(
	//! A target for delayed message.
	Target && target,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	Args&&... args )
	{
		using namespace send_functions_details;

		send_delayed< Message >(
				arg_to_env( target ),
				arg_to_mbox( target ),
				pause,
				std::forward< Args >(args)... );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a delayed message.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
void
send_delayed(
	//! An agent whos environment must be used.
	so_5::agent_t & agent,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	Args&&... args )
	{
		send_delayed< Message >( agent.so_environment(), to, pause,
				std::forward< Args >(args)... );
	}

/*!
 * \brief A utility function for delayed redirection of a message
 * from existing message hood.
 *
 * \tparam Message a type of message to be redirected (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			so_5::send_delayed(so_environment(), another_mbox, std::chrono::seconds(1), cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			so_5::send_delayed(so_environment(), another_mbox, std::chrono::seconds(1), std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Message >
typename std::enable_if< !message_payload_type<Message>::is_signal >::type
send_delayed(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message instance owner.
	mhood_t< Message > msg )
	{
		env.single_timer(
				message_payload_type< Message >::subscription_type_index(),
				msg.make_reference(),
				to,
				pause );
	}

/*!
 * \brief A utility function for delayed redirection of a signal
 * from existing message hood.
 *
 * \tparam Message a type of signal to be redirected (it can be
 * in form of Sig or so_5::immutable_msg<Sig>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_signal(mhood_t<some_signal> cmd) {
			so_5::send_delayed(so_environment(), another_mbox, std::chrono::seconds(1), cmd);
			...
		}
	};
 * \endcode
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Message >
typename std::enable_if< message_payload_type<Message>::is_signal >::type
send_delayed(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message instance owner.
	mhood_t< Message > /*msg*/ )
	{
		env.single_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t{},
				to,
				pause );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a delayed message
 * to the agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
void
send_delayed_to_agent(
	//! An agent whos environment must be used.
	so_5::agent_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	Args&&... args )
	{
		send_delayed< Message >(
				agent.so_environment(),
				agent.so_direct_mbox(),
				pause,
				std::forward< Args >(args)... );
	}

/*!
 * \brief A helper function for redirection of a message/signal as a delayed
 * message/signal.
 *
 * This function can be used if \a target is a reference to agent or if
 * \a target is a mchain. In such cases instead of writing:
 * \code
 * periodic_id = so_5::send_delayed(
 * 		target_agent.so_environment(),
 * 		target_agent.so_direct_mbox(),
 * 		pause,
 * 		std::move(msg));
 * \endcode
 * it is possible to write:
 * \code
 * periodic_id = so_5::send_delayed(
 * 		target_agent,
 * 		pause,
 * 		std::move(msg));
 * \endcode
 * 
 * Example usage:
 * \code
 * class my_agent : public so_5::agent_t {
 * ...
 * 	so_5::mchain_t target_mchain_;
 * ...
 * 	void on_some_msg(mhood_t<some_msg> cmd) {
 * 		if( ... )
 * 			// Message should be resend as a delayed message.
 * 			so_5::send_delayed(target_mchain_, 10s, 20s, std::move(cmd));
 * 	}
 * \endcode
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \since
 * v.5.5.20
 */
template< typename Target, typename Message >
void
send_delayed(
	//! A target for delayed message/signal.
	//! It can be a reference to a target agent or a mchain_t.
	Target && target,
	//! Pause for the next occurence of the message/signal.
	std::chrono::steady_clock::duration pause,
	//! Existing message hood for message/signal to be sent.
	mhood_t< Message > mhood )
	{
		using namespace send_functions_details;
		send_delayed< Message >(
				arg_to_env( target ),
				arg_to_mbox( target ),
				pause,
				std::move(mhood) );
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief A utility function for creating and delivering a delayed message
 * to the ad-hoc agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
void
send_delayed_to_agent(
	//! An agent whos environment must be used.
	const so_5::adhoc_agent_definition_proxy_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message constructor parameters.
	Args&&... args )
	{
		send_delayed< Message >(
				agent.environment(),
				agent.direct_mbox(),
				pause,
				std::forward< Args >(args)... );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a periodic message.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 */
template< typename Message, typename... Args >
SO_5_NODISCARD timer_id_t
send_periodic(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Message constructor parameters.
	Args&&... args )
	{
		return so_5::impl::instantiator_and_sender< Message >::send_periodic(
				env, to, pause, period, std::forward< Args >( args )... );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a periodic message.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
SO_5_NODISCARD timer_id_t
send_periodic(
	//! An agent whos environment must be used.
	so_5::agent_t & agent,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Message constructor parameters.
	Args&&... args )
	{
		return send_periodic< Message >(
				agent.so_environment(),
				to,
				pause,
				period,
				std::forward< Args >(args)... );
	}

/*!
 * \brief A utility function for creating and delivering a periodic message
 * to the specified destination.
 *
 * Agent, ad-hoc agent or mchain can be used as \a target.
 *
 * \note
 * Message chains with overload control must be used for periodic messages
 * with additional care: \ref so_5_5_18__overloaded_mchains_and_timers.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \tparam Message type of message or signal to be sent.
 * \tparam Target can be so_5::agent_t, so_5::adhoc_agent_definition_proxy_t or
 * so_5::mchain_t.
 * \tparam Args list of arguments for Message's constructor.
 *
 * \since
 * v.5.5.19
 */
template< typename Message, typename Target, typename... Args >
SO_5_NODISCARD timer_id_t
send_periodic(
	//! A destination for the periodic message.
	Target && target,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Message constructor parameters.
	Args&&... args )
	{
		using namespace send_functions_details;
		return send_periodic< Message >(
				arg_to_env( target ),
				arg_to_mbox( target ),
				pause,
				period,
				std::forward< Args >(args)... );
	}

/*!
 * \brief A utility function for delivering a periodic
 * from an existing message hood.
 *
 * \attention Message must not be a mutable message if \a period is not 0.
 * Otherwise an exception will be thrown.
 *
 * \tparam Message a type of message to be redirected (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			timer_id = so_5::send_periodic(so_environment(), another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds(15),
					cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			timer_id = so_5::send_periodic(so_environment(), another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds::zero(), // Note: period is 0!
					std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Message >
SO_5_NODISCARD typename std::enable_if< !is_signal< Message >::value, timer_id_t >::type
send_periodic(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Existing message hood for message to be sent.
	mhood_t< Message > mhood )
	{
		return env.schedule_timer( 
				message_payload_type< Message >::subscription_type_index(),
				mhood.make_reference(),
				to,
				pause,
				period );
	}

/*!
 * \brief A utility function for periodic redirection of a signal
 * from existing message hood.
 *
 * \tparam Message a type of signal to be redirected (it can be
 * in form of Sig or so_5::immutable_msg<Sig>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_signal(mhood_t<some_signal> cmd) {
			timer_id = so_5::send_periodic(so_environment(), another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds(10),
					cmd);
			...
		}
	};
 * \endcode
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Message >
SO_5_NODISCARD typename std::enable_if< is_signal< Message >::value, timer_id_t >::type
send_periodic(
	//! An environment to be used for timer.
	so_5::environment_t & env,
	//! Mbox for the message to be sent to.
	const so_5::mbox_t & to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Existing message hood for message to be sent.
	mhood_t< Message > /*mhood*/ )
	{
		return env.schedule_timer( 
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t{},
				to,
				pause,
				period );
	}

/*!
 * \brief A helper function for redirection of a message/signal as a periodic
 * message/signal.
 *
 * This function can be used if \a target is a reference to agent or if
 * \a target is a mchain. In such cases instead of writing:
 * \code
 * periodic_id = so_5::send_periodic(
 * 		target_agent.so_environment(),
 * 		target_agent.so_direct_mbox(),
 * 		pause,
 * 		period,
 * 		std::move(msg));
 * \endcode
 * it is possible to write:
 * \code
 * periodic_id = so_5::send_periodic(
 * 		target_agent,
 * 		pause,
 * 		period,
 * 		std::move(msg));
 * \endcode
 * 
 * Example usage:
 * \code
 * class my_agent : public so_5::agent_t {
 * ...
 * 	so_5::mchain_t target_mchain_;
 * 	so_5::timer_id_t periodic_msg_id_;
 * ...
 * 	void on_some_msg(mhood_t<some_msg> cmd) {
 * 		if( ... )
 * 			// Message should be resend as a periodic message.
 * 			periodic_msg_id_ = so_5::send_periodic(target_mchain_, 10s, 20s, std::move(cmd));
 * 	}
 * \endcode
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \since
 * v.5.5.20
 */
template< typename Target, typename Message >
SO_5_NODISCARD timer_id_t
send_periodic(
	//! A target for periodic message/signal.
	//! It can be a reference to a target agent or a mchain_t.
	Target && target,
	//! Pause for the first occurence of the message/signal.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Existing message hood for message/signal to be sent.
	mhood_t< Message > mhood )
	{
		using namespace send_functions_details;
		return send_periodic< Message >(
				arg_to_env( target ),
				arg_to_mbox( target ),
				pause,
				period,
				std::move(mhood) );
	}

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a periodic message
 * to the agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
SO_5_NODISCARD timer_id_t
send_periodic_to_agent(
	//! An agent whos environment must be used.
	so_5::agent_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Message constructor parameters.
	Args&&... args )
	{
		return send_periodic< Message >(
				agent.so_environment(),
				agent.so_direct_mbox(),
				pause,
				period,
				std::forward< Args >(args)... );
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief A utility function for creating and delivering a periodic message
 * to the agent's direct mbox.
 *
 * Gets the Environment from the agent specified.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \deprecated Will be removed in v.5.6.0.
 */
template< typename Message, typename... Args >
SO_5_NODISCARD timer_id_t
send_periodic_to_agent(
	//! An agent whos environment must be used.
	const so_5::adhoc_agent_definition_proxy_t & agent,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Message constructor parameters.
	Args&&... args )
	{
		return send_periodic< Message >(
				agent.environment(),
				agent.direct_mbox(),
				pause,
				period,
				std::forward< Args >(args)... );
	}

/*!
 * \name Helper functions for simplification of synchronous interactions.
 * \{
 */

/*!
 * \since
 * v.5.5.9
 *
 * \brief Make a synchronous request and receive result in form of a future
 * object. Intended to use with messages.
 *
 * \tparam Result type of expected result. The std::future<Result> will be
 * returned.
 * \tparam Msg type of message to be sent to request processor.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t or
 * so_5::adhoc_agent_definition_proxy_t (in two later cases agent's direct
 * mbox will be used).
 * \tparam Args arguments for Msg's constructors.
 *
 * \par Usage example:
 * \code
	// For sending request to mbox:
	const so_5::mbox_t & convert_mbox = ...;
	auto f1 = so_5::request_future< std::string, int >( convert_mbox, 10 );
	...
	f1.get();

	// For sending request to agent:
	const so_5::agent_t & a = ...;
	auto f2 = so_5::request_future< std::string, int >( a, 10 );
	...
	f2.get();

	// For sending request to ad-hoc agent:
	auto service = coop.define_agent();
	coop.define_agent().on_start( [service] {
		auto f3 = so_5::request_future< std::string, int >( service, 10 );
		...
		f3.get();
	} );
 * \endcode
 */
template< typename Result, typename Msg, typename Target, typename... Args >
std::future< Result >
request_future(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Arguments for Msg's constructor params.
	Args &&... args )
	{
		using namespace send_functions_details;

		so_5::ensure_not_signal< Msg >();

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.template make_async< Msg >( std::forward< Args >(args)... );
	}

/*!
 * \brief A version of %request_future function for initiating of a
 * synchonous request from exising message hood.
 *
 * \tparam Result type of an expected result.
 * \tparam Target type of a destination (it can be agent, adhoc-agent,
 * mbox or mchain).
 * \tparam Msg type of a message to be used as request (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			auto f = so_5::request_future<result>(another_mbox, cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			auto f = so_5::request_future<result>(another_mbox, std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template< typename Result, typename Msg, typename Target >
typename std::enable_if< !is_signal<Msg>::value, std::future<Result> >::type
request_future(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Already existing message.
	mhood_t< Msg > mhood )
	{
		using namespace send_functions_details;

		so_5::ensure_not_signal< Msg >();

		using subscription_type =
				typename message_payload_type<Msg>::subscription_type;

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.template async_2< subscription_type >( mhood.make_reference() );
	}

/*!
 * \brief A version of %request_future function for initiating of a
 * synchonous request from exising message hood.
 *
 * \tparam Result type of an expected result.
 * \tparam Target type of a destination (it can be agent, adhoc-agent,
 * mbox or mchain).
 * \tparam Msg type of a signal to be used as request (it can be
 * in form of Msg or so_5::immutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_signal(mhood_t<some_signal> cmd) {
			auto f = so_5::request_future<result>(another_mbox, cmd);
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template< typename Result, typename Msg, typename Target >
typename std::enable_if< is_signal<Msg>::value, std::future<Result> >::type
request_future(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Already existing message.
	mhood_t< Msg > /*mhood*/ )
	{
		using namespace send_functions_details;

		so_5::ensure_signal< Msg >();

		using subscription_type =
				typename message_payload_type<Msg>::subscription_type;

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.template async< subscription_type >();
	}

/*!
 * \since
 * v.5.5.9
 *
 * \brief Make a synchronous request and receive result in form of a future
 * object. Intended to use with signals.
 *
 * \tparam Result type of expected result. The std::future<Result> will be
 * returned.
 * \tparam Signal type of signal to be sent to request processor.
 * This type must be derived from so_5::signal_t.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t or
 * so_5::adhoc_agent_definition_proxy_t (in two later cases agent's direct
 * mbox will be used).
 * \tparam Future_Type type of funtion return value (detected automatically).
 *
 * \par Usage example:
 * \code
	struct get_status : public so_5::signal_t {};

	// For sending request to mbox:
	const so_5::mbox_t & engine = ...;
	auto f1 = so_5::request_future< std::string, get_status >( engine );
	...
	f1.get();

	// For sending request to agent:
	const so_5::agent_t & engine = ...;
	auto f2 = so_5::request_future< std::string, get_status >( engine );
	...
	f2.get();

	// For sending request to ad-hoc agent:
	auto engine = coop.define_agent();
	coop.define_agent().on_start( [engine] {
		auto f3 = so_5::request_future< std::string, get_status >( engine );
		...
		f3.get();
	} );
 * \endcode
 */
template<
		typename Result,
		typename Signal,
		typename Target,
		typename Future_Type =
				typename std::enable_if<
						so_5::is_signal< Signal >::value, std::future< Result >
				>::type >
Future_Type
request_future(
	//! Target for sending a synchronous request to.
	Target && who )
	{
		using namespace send_functions_details;

		so_5::ensure_signal< Signal >();

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.template async< Signal >();
	}

/*!
 * \since
 * v.5.5.9
 *
 * \brief Make a synchronous request and receive result in form of a value
 * with waiting for some time. Intended to use with messages.
 *
 * \tparam Result type of expected result.
 * \tparam Msg type of message to be sent to request processor.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t or
 * so_5::adhoc_agent_definition_proxy_t (in two later cases agent's direct
 * mbox will be used).
 * \tparam Duration type of waiting indicator. Can be
 * so_5::service_request_infinite_waiting_t or some of std::chrono type.
 * \tparam Args arguments for Msg's constructors.
 *
 * \par Usage example:
 * \code
	// For sending request to mbox:
	const so_5::mbox_t & convert_mbox = ...;
	auto r1 = so_5::request_value< std::string, int >( convert_mbox, so_5::infinite_wait, 10 );
	auto r2 = so_5::request_value< std::string, int >( convert_mbox, std::chrono::milliseconds(10), 10 );

	// For sending request to agent:
	const so_5::agent_t & a = ...;
	auto r3 = so_5::request_value< std::string, int >( a, so_5::infinite_wait, 10 );
	auto r4 = so_5::request_value< std::string, int >( a, std::chrono::milliseconds(10), 10 );

	// For sending request to ad-hoc agent:
	auto service = coop.define_agent();
	coop.define_agent().on_start( [service] {
		auto r5 = so_5::request_value< std::string, int >( service, so_5::infinite_wait, 10 );
		auto r6 = so_5::request_value< std::string, int >( service, std::chrono::milliseconds(10), 10 );
	} );
 * \endcode
 */
template<
		typename Result,
		typename Msg,
		typename Target,
		typename Duration,
		typename... Args >
Result
request_value(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Time to wait.
	Duration timeout,
	//! Arguments for Msg's constructor params.
	Args &&... args )
	{
		using namespace send_functions_details;

		so_5::ensure_not_signal< Msg >();

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.get_wait_proxy( timeout )
				.template make_sync_get< Msg >( std::forward< Args >(args)... );
	}

/*!
 * \brief A version of %request_value function for initiating of a
 * synchonous request from exising message hood.
 *
 * \tparam Result type of an expected result.
 * \tparam Target type of a destination (it can be agent, adhoc-agent,
 * mbox or mchain).
 * \tparam Duration type of waiting indicator. Can be
 * so_5::service_request_infinite_waiting_t or some of std::chrono type.
 * \tparam Msg type of a message to be used as request (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			auto r = so_5::request_value<result>(another_mbox, so_5::infinite_wait cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			auto r = so_5::request_value<result>(another_mbox, std::chrono::seconds(5), std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template<
		typename Result,
		typename Target,
		typename Duration,
		typename Msg >
typename std::enable_if< !so_5::is_signal<Msg>::value, Result >::type
request_value(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Time to wait.
	Duration timeout,
	//! Message hood with existed message instance.
	mhood_t< Msg > mhood )
	{
		using namespace send_functions_details;

		so_5::ensure_not_signal< Msg >();

		using subscription_type = typename message_payload_type<Msg>::subscription_type;

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.get_wait_proxy( timeout )
				.template sync_get_2< subscription_type >( mhood.make_reference() );
	}

/*!
 * \since
 * v.5.5.9
 *
 * \brief Make a synchronous request and receive result in form of a value with
 * waiting for some time. Intended to use with signals.
 *
 * \tparam Result type of expected result.
 * returned.
 * \tparam Signal type of signal to be sent to request processor.
 * This type must be derived from so_5::signal_t.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t or
 * so_5::adhoc_agent_definition_proxy_t (in two later cases agent's direct
 * mbox will be used).
 * \tparam Duration type of waiting indicator. Can be
 * so_5::service_request_infinite_waiting_t or some of std::chrono type.
 * \tparam Result_Type type of funtion return value (detected automatically).
 *
 * \par Usage example:
 * \code
	struct get_status : public so_5::signal_t {};

	// For sending request to mbox:
	const so_5::mbox_t & engine = ...;
	auto r1 = so_5::request_value< std::string, get_status >( engine, so_5::infinite_wait );
	auto r2 = so_5::request_value< std::string, get_status >( engine, std::chrono::milliseconds(10) );

	// For sending request to agent:
	const so_5::agent_t & engine = ...;
	auto r3 = so_5::request_value< std::string, get_status >( engine, so_5::infinite_wait );
	auto r4 = so_5::request_value< std::string, get_status >( engine, std::chrono::milliseconds(10) );

	// For sending request to ad-hoc agent:
	auto engine = coop.define_agent();
	coop.define_agent().on_start( [engine] {
		auto r5 = so_5::request_value< std::string, get_status >( engine, so_5::infinite_wait );
		auto r6 = so_5::request_value< std::string, get_status >( engine, std::chrono::milliseconds(10) );
	} );
 * \endcode
 */
template<
		typename Result,
		typename Signal,
		typename Target,
		typename Duration,
		typename Result_Type =
				typename std::enable_if<
						so_5::is_signal< Signal >::value, Result
				>::type >
Result_Type
request_value(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Time for waiting for a result.
	Duration timeout )
	{
		using namespace send_functions_details;

		so_5::ensure_signal< Signal >();

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.get_wait_proxy( timeout )
				.template sync_get< Signal >();
	}

/*!
 * \brief A version of %request_value function for initiating of a
 * synchonous request from exising message hood.
 *
 * Intended to be used with signals.
 *
 * \tparam Result type of an expected result.
 * \tparam Target type of a destination (it can be agent, adhoc-agent,
 * mbox or mchain).
 * \tparam Duration type of waiting indicator. Can be
 * so_5::service_request_infinite_waiting_t or some of std::chrono type.
 * \tparam Msg type of a signal to be used as request (it can be
 * in form of Msg or so_5::immutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_signal(mhood_t<some_signal> cmd) {
			auto r = so_5::request_value<result>(another_mbox, so_5::infinite_wait, cmd);
			...
		}
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template<
		typename Result,
		typename Target,
		typename Duration,
		typename Msg >
typename std::enable_if< so_5::is_signal<Msg>::value, Result >::type
request_value(
	//! Target for sending a synchronous request to.
	Target && who,
	//! Time to wait.
	Duration timeout,
	//! Message hood with existed message instance.
	mhood_t< Msg > /*mhood*/ )
	{
		using namespace send_functions_details;

		using subscription_type = typename message_payload_type<Msg>::subscription_type;

		so_5::ensure_signal< subscription_type >();

		return arg_to_mbox( std::forward< Target >(who) )
				->template get_one< Result >()
				.get_wait_proxy( timeout )
				.template sync_get< subscription_type >();
	}
/*!
 * \}
 */

} /* namespace so_5 */


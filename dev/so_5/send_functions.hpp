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

#include <so_5/environment.hpp>

#include <so_5/compiler_features.hpp>

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
		private :
			// Helper method for message instance creation and
			// mutability flag handling.
			template< typename... Args >
			static auto
			make_instance( Args &&... args )
				{
					// it will be std::unique_ptr<Envelope>, where Envelope
					// can be a different type. But Envelope is derived from
					// so_5::message_t.
					auto msg_instance =
						so_5::details::make_message_instance< Message >(
								std::forward< Args >( args )...);
					so_5::details::mark_as_mutable_if_necessary< Message >(
							*msg_instance );

					return msg_instance;
				}

		public :
			template< typename... Args >
			static void
			send(
				const so_5::mbox_t & to,
				Args &&... args )
				{
					so_5::low_level_api::deliver_message(
							*to,
							message_payload_type< Message >::subscription_type_index(),
							make_instance( std::forward<Args>(args)... ) );
				}

			template< typename... Args >
			static void
			send_delayed(
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				Args &&... args )
				{
					so_5::low_level_api::single_timer(
							message_payload_type< Message >::subscription_type_index(),
							message_ref_t{ make_instance( std::forward<Args>(args)... ) },
							to,
							pause );
				}

			template< typename... Args >
			SO_5_NODISCARD static timer_id_t
			send_periodic(
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				std::chrono::steady_clock::duration period,
				Args &&... args )
				{
					return so_5::low_level_api::schedule_timer( 
							message_payload_type< Message >::subscription_type_index(),
							message_ref_t{ make_instance( std::forward<Args>(args)... ) },
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
					using namespace so_5::low_level_api;
					deliver_signal< actual_signal_type >( *to );
				}

			static void
			send_delayed(
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause )
				{
					so_5::low_level_api::single_timer(
							message_payload_type<Message>::subscription_type_index(),
							message_ref_t{},
							to,
							pause );
				}

			SO_5_NODISCARD static timer_id_t
			send_periodic(
				const so_5::mbox_t & to,
				std::chrono::steady_clock::duration pause,
				std::chrono::steady_clock::duration period )
				{
					return so_5::low_level_api::schedule_timer( 
							message_payload_type< Message >::subscription_type_index(),
							message_ref_t{},
							to,
							pause,
							period );
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

inline so_5::mbox_t
arg_to_mbox( const so_5::mchain_t & chain ) { return chain->as_mbox(); }

inline so_5::environment_t &
arg_to_env( const so_5::mbox_t & mbox ) { return mbox->environment(); }

inline so_5::environment_t &
arg_to_env( const so_5::agent_t & agent ) { return agent.so_environment(); }

inline so_5::environment_t &
arg_to_env( const so_5::mchain_t & chain ) { return chain->environment(); }

} /* namespace send_functions_details */

/*!
 * \since
 * v.5.5.1
 *
 * \brief A utility function for creating and delivering a message or a signal.
 *
 * \note Since v.5.5.13 can send also a signal.
 *
 * \tparam Message type of message to be sent.
 * \tparam Target identification of request processor. Could be reference to
 * so_5::mbox_t, to so_5::agent_t (the later case agent's direct
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
		using namespace so_5::low_level_api;

		deliver_message(
				*send_functions_details::arg_to_mbox( std::forward<Target>(to) ),
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
 * \brief A utility function for creating and delivering a delayed message
 * to the specified destination.
 *
 * Agent or mchain can be used as \a target.
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \tparam Message type of message or signal to be sent.
 * \tparam Target can be so_5::agent_t or so_5::mchain_t.
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

		so_5::impl::instantiator_and_sender< Message >::send_delayed(
				arg_to_mbox( target ),
				pause,
				std::forward< Args >(args)... );
	}

/*!
 * \brief A utility function for delayed redirection of a message
 * from existing message hood.
 *
 * \tparam Target a type of destination of the message. It can be an agent,
 * a mbox or mchain.
 * \tparam Message a type of message to be redirected (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			so_5::send_delayed(another_mbox, std::chrono::seconds(1), cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			so_5::send_delayed(another_mbox, std::chrono::seconds(1), std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 * A redirection to the direct mbox of some agent can looks like:
 * \code
 * void some_agent::on_some_message(mhood_t<some_message> cmd) {
 * 	// Redirect to itself but with a pause.
 * 	so_5::send_delayed(*this, std::chrono::seconds(2), cmd);
 * }
 * // For the case of mutable message.
 * void another_agent::on_another_msg(mutable_mhood_t<another_msg> cmd) {
 * 	// Redirect to itself but with a pause.
 * 	so_5::send_delayed(*this, std::chrono::seconds(2), std::move(cmd));
 * 	// Note: cmd is nullptr now, it can't be used anymore.
 * }
 * \endcode
 * A redirection to a mchain can looks like:
 * \code
 * so_5::mchain_t first = ...;
 * so_5::mchain_t second = ...;
 * so_5::receive(so_5::from(first).handle_n(1),
 * 	[second](so_5::mhood_t<some_message> cmd) {
 * 		so_5::send_delayed(second, std::chrono::seconds(1), cmd);
 * 	},
 * 	[second](so_5::mutable_mhood_t<another_message> cmd) {
 * 		so_5::send_delayed(second, std::chrono::seconds(1), std::move(cmd));
 * 		// Note: cmd is nullptr now, it can't be used anymore.
 * 	});
 * \endcode
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
typename std::enable_if< !message_payload_type<Message>::is_signal >::type
send_delayed(
	//! Destination for the message.
	Target && to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message instance owner.
	mhood_t< Message > msg )
	{
		using namespace send_functions_details;

		so_5::low_level_api::single_timer(
				message_payload_type< Message >::subscription_type_index(),
				msg.make_reference(),
				arg_to_mbox( to ),
				pause );
	}

/*!
 * \brief A utility function for delayed redirection of a signal
 * from existing message hood.
 *
 * \tparam Target a type of destination of the signal. It can be an agent,
 * a mbox or mchain.
 * \tparam Message a type of signal to be redirected (it can be
 * in form of Sig or so_5::immutable_msg<Sig>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_signal(mhood_t<some_signal> cmd) {
			so_5::send_delayed(another_mbox, std::chrono::seconds(1), cmd);
			...
		}
	};
 * \endcode
 * A redirection to the direct mbox of some agent can looks like:
 * \code
 * void some_agent::on_some_signal(mhood_t<some_signal> cmd) {
 * 	// Redirect to itself but with a pause.
 * 	so_5::send_delayed(*this, std::chrono::seconds(2), cmd);
 * }
 * \endcode
 * A redirection to a mchain can looks like:
 * \code
 * so_5::mchain_t first = ...;
 * so_5::mchain_t second = ...;
 * so_5::receive(so_5::from(first).handle_n(1),
 * 	[second](so_5::mhood_t<some_signal> cmd) {
 * 		so_5::send_delayed(second, std::chrono::seconds(1), cmd);
 * 	});
 * \endcode
 *
 * \attention
 * Value of \a pause should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
typename std::enable_if< message_payload_type<Message>::is_signal >::type
send_delayed(
	//! Destination for the message.
	Target && to,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Message instance owner.
	mhood_t< Message > /*msg*/ )
	{
		using namespace send_functions_details;

		so_5::low_level_api::single_timer(
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t{},
				arg_to_mbox( to ),
				pause );
	}

/*!
 * \brief A utility function for creating and delivering a periodic message
 * to the specified destination.
 *
 * Agent or mchain can be used as \a target.
 *
 * \note
 * Message chains with overload control must be used for periodic messages
 * with additional care: \ref so_5_5_18__overloaded_mchains_and_timers.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \tparam Message type of message or signal to be sent.
 * \tparam Target can be so_5::agent_t or so_5::mchain_t.
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

		return so_5::impl::instantiator_and_sender< Message >::send_periodic(
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
 * \tparam Target a type of destination of the message. It can be an agent,
 * a mbox or mchain.
 * \tparam Message a type of message to be redirected (it can be
 * in form of Msg, so_5::immutable_msg<Msg> or so_5::mutable_msg<Msg>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_message(mhood_t<first_msg> cmd) {
			timer_id = so_5::send_periodic(another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds(15),
					cmd);
			...
		}

		void on_some_mutable_message(mhood_t<mutable_msg<second_msg>> cmd) {
			timer_id = so_5::send_periodic(another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds::zero(), // Note: period is 0!
					std::move(cmd));
			// Note: cmd is nullptr now, it can't be used anymore.
			...
		}
	};
 * \endcode
 * A redirection to the direct mbox of some agent can looks like:
 * \code
 * void some_agent::on_some_message(mhood_t<some_message> cmd) {
 * 	// Redirect to itself but with a pause and a period.
 * 	timer_id = so_5::send_periodic(*this,
 * 			std::chrono::seconds(2),
 * 			std::chrono::seconds(5),
 * 			cmd);
 * }
 * \endcode
 * A redirection to a mchain can looks like:
 * \code
 * so_5::mchain_t first = ...;
 * so_5::mchain_t second = ...;
 * so_5::timer_id_t timer_id;
 * so_5::receive(so_5::from(first).handle_n(1),
 * 	[&](so_5::mhood_t<some_message> cmd) {
 * 		timer_id = so_5::send_periodic(
 * 				second,
 * 				std::chrono::seconds(2),
 * 				std::chrono::seconds(5),
 * 				cmd);
 * 	});
 * \endcode
 * \note
 * Message chains with overload control must be used for periodic messages
 * with additional care: \ref so_5_5_18__overloaded_mchains_and_timers.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
SO_5_NODISCARD typename std::enable_if< !is_signal< Message >::value, timer_id_t >::type
send_periodic(
	//! A destination for the periodic message.
	Target && target,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Existing message hood for message to be sent.
	mhood_t< Message > mhood )
	{
		using namespace send_functions_details;

		return so_5::low_level_api::schedule_timer( 
				message_payload_type< Message >::subscription_type_index(),
				mhood.make_reference(),
				arg_to_mbox( target ),
				pause,
				period );
	}

/*!
 * \brief A utility function for periodic redirection of a signal
 * from existing message hood.
 *
 * \tparam Target a type of destination of the signal. It can be an agent,
 * a mbox or mchain.
 * \tparam Message a type of signal to be redirected (it can be
 * in form of Sig or so_5::immutable_msg<Sig>).
 *
 * Usage example:
 * \code
	class redirector : public so_5::agent_t {
		...
		void on_some_immutable_signal(mhood_t<some_signal> cmd) {
			timer_id = so_5::send_periodic(another_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds(10),
					cmd);
			...
		}
	};
 * \endcode
 * A redirection to the direct mbox of some agent can looks like:
 * \code
 * void some_agent::on_some_message(mhood_t<some_signal> cmd) {
 * 	// Redirect to itself but with a pause and a period.
 * 	timer_id = so_5::send_periodic(*this,
 * 			std::chrono::seconds(2),
 * 			std::chrono::seconds(5),
 * 			cmd);
 * }
 * \endcode
 * A redirection to a mchain can looks like:
 * \code
 * so_5::mchain_t first = ...;
 * so_5::mchain_t second = ...;
 * so_5::timer_id_t timer_id;
 * so_5::receive(so_5::from(first).handle_n(1),
 * 	[&](so_5::mhood_t<some_signal> cmd) {
 * 		timer_id = so_5::send_periodic(
 * 				second,
 * 				std::chrono::seconds(2),
 * 				std::chrono::seconds(5),
 * 				cmd);
 * 	});
 * \endcode
 * \note
 * Message chains with overload control must be used for periodic messages
 * with additional care: \ref so_5_5_18__overloaded_mchains_and_timers.
 *
 * \attention
 * Values of \a pause and \a period should be non-negative.
 *
 * \since
 * v.5.5.19
 */
template< typename Target, typename Message >
SO_5_NODISCARD typename std::enable_if< is_signal< Message >::value, timer_id_t >::type
send_periodic(
	//! A destination for the periodic message.
	Target && target,
	//! Pause for message delaying.
	std::chrono::steady_clock::duration pause,
	//! Period of message repetitions.
	std::chrono::steady_clock::duration period,
	//! Existing message hood for message to be sent.
	mhood_t< Message > /*mhood*/ )
	{
		using namespace send_functions_details;

		return so_5::low_level_api::schedule_timer( 
				message_payload_type< Message >::subscription_type_index(),
				message_ref_t{},
				arg_to_mbox( target ),
				pause,
				period );
	}

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.13
 *
 * \file
 * \brief Various stuff for creation of event handlers.
 */

#pragma once

#include <so_5/h/exception.hpp>
#include <so_5/h/ret_code.hpp>

#include <so_5/details/h/lambda_traits.hpp>

#include <so_5/rt/h/execution_demand.hpp>
#include <so_5/rt/h/message_handler_format_detector.hpp>
#include <so_5/rt/h/mbox.hpp>

#include <algorithm>

namespace so_5 {

namespace details {

/*!
 * \brief Various helpers for event subscription.
 *
 * \since
 * v.5.3.0
 */
namespace event_subscription_helpers
{

/*!
 * \brief Get actual agent pointer.
 *
 * \throw exception_t if dynamic_cast fails.
 */
template< class Agent >
Agent *
get_actual_agent_pointer( agent_t & agent )
{
	// Agent must have right type.
	Agent * cast_result = dynamic_cast< Agent * >( &agent );

	// Was conversion successful?
	if( nullptr == cast_result )
	{
		// No. Actual type of the agent is not convertible to the Agent.
		SO_5_THROW_EXCEPTION(
			rc_agent_incompatible_type_conversion,
			std::string( "Unable convert agent to type: " ) +
				typeid(Agent).name() );
	}

	return cast_result;
}

/*!
 * \brief Get actual msg_service_request pointer.
 *
 * \throw exception_t if dynamic_cast fails.
 */
template< class Result, class Message >
msg_service_request_t<
		Result,
		typename message_payload_type< Message >::envelope_type > *
get_actual_service_request_pointer(
	const message_ref_t & message_ref )
{
	using actual_request_msg_t =
			msg_service_request_t<
					Result,
					typename message_payload_type< Message >::envelope_type >;

	auto actual_request_ptr = dynamic_cast< actual_request_msg_t * >(
			message_ref.get() );

	if( !actual_request_ptr )
		SO_5_THROW_EXCEPTION(
				rc_msg_service_request_bad_cast,
				std::string( "unable cast msg_service_request "
						"instance to appropriate type, "
						"expected type is: " ) +
				typeid(actual_request_msg_t).name() );

	return actual_request_ptr;
}

/*!
 * \brief A helper template for create an argument for event handler
 * in the case when argument is passed as value or const reference.
 *
 * \note Msg can't be a type of a signal.
 *
 * \since
 * v.5.5.14
 */
template< typename Msg >
struct event_handler_arg_maker
{
	using traits_type = message_payload_type< Msg >;

	static void
	ensure_appropriate_type()
	{
		ensure_not_signal< Msg >();
	}

	static const Msg &
	make_arg( message_ref_t & mf )
	{
		auto msg = message_payload_type< Msg >::extract_payload_ptr( mf );

		return *msg;
	}
};

/*!
 * \brief A helper template for create an argument for event handler
 * in the case when argument is passed as message hood.
 * \since
 * v.5.5.14
 */
template< typename Msg >
struct event_handler_arg_maker< mhood_t< Msg > >
{
	using traits_type = message_payload_type< Msg >;

	static void
	ensure_appropriate_type()
	{
	}

	static mhood_t< Msg >
	make_arg( message_ref_t & mf )
	{
		return mhood_t< Msg >{ mf };
	}
};

/*!
 * \brief A helper template for create an argument for event handler
 * in the case when argument is passed as message hood.
 *
 * \since
 * v.5.5.19
 */
template< typename Msg >
struct event_handler_arg_maker< mhood_t< immutable_msg<Msg> > >
{
	using traits_type = message_payload_type< immutable_msg<Msg> >;

	static void
	ensure_appropriate_type()
	{
	}

	static mhood_t< immutable_msg<Msg> >
	make_arg( message_ref_t & mf )
	{
		return { mf };
	}
};

/*!
 * \brief A helper template for create an argument for event handler
 * in the case when argument is passed as message hood.
 *
 * \since
 * v.5.5.19
 */
template< typename Msg >
struct event_handler_arg_maker< mhood_t< mutable_msg<Msg> > >
{
	using traits_type = message_payload_type< mutable_msg< Msg > >;

	static void
	ensure_appropriate_type()
	{
		ensure_not_signal<Msg>();
	}

	static mhood_t< mutable_msg<Msg> >
	make_arg( message_ref_t & mf )
	{
		return { mf };
	}
};

/*!
 * \brief A helper template type to trigger static_assert.
 *
 * \since
 * v.5.5.19
 */
template<typename T>
struct always_false
{
	static const bool value = false;
};

/*
 * Disable usage of mutable_msg<Msg> as parameter of an event handler.
 */
template< typename Msg >
struct event_handler_arg_maker< mutable_msg<Msg> >
{
	using traits_type = message_payload_type< mutable_msg<Msg> >;

	static void
	ensure_appropriate_type()
	{
		static_assert(always_false<Msg>::value,
				"mutable_msg<T> can't be used as type of event handler parameter");
	}

	static mutable_msg<Msg>
	make_arg( message_ref_t & ) { return {}; }
};

/*
 * Disable usage of mutable_msg<Msg> as parameter of an event handler.
 */
template< typename Msg >
struct event_handler_arg_maker< immutable_msg<Msg> >
{
	using traits_type = message_payload_type< immutable_msg<Msg> >;

	static void
	ensure_appropriate_type()
	{
		static_assert(always_false<Msg>::value,
				"immutable_msg<T> can't be used as type of event handler parameter");
	}

	static immutable_msg<Msg>
	make_arg( message_ref_t & ) { return {}; }
};


/*!
 * \brief A helper for setting a result to a promise.
 * \since
 * v.5.5.14
 */
template< typename R, typename L >
void
set_promise( std::promise< R > & to, L result_provider )
{
	to.set_value( result_provider() );
}

/*!
 * \since
 * v.5.5.14
 *
 * \brief A helper for setting a result to a promise<void>.
 */
template< typename L >
void
set_promise( std::promise< void > & to, L result_provider )
{
	result_provider();
	to.set_value();
}

/*!
 * \brief Helper template for creation of event handler with actual
 * argument.
 *
 * \since
 * v.5.5.14
 */
template< typename Lambda, typename Result, typename Arg >
typename std::enable_if<
		!is_agent_method_pointer<Lambda>::value,
		msg_type_and_handler_pair_t >::type
make_handler_with_arg( Lambda lambda )
	{
		using arg_maker = event_handler_arg_maker< Arg >;
		using payload_type = typename arg_maker::traits_type::payload_type;

		arg_maker::ensure_appropriate_type();

		auto method = [lambda](
				invocation_type_t invocation_type,
				message_ref_t & message_ref) mutable
			{
				if( invocation_type_t::service_request == invocation_type )
					{
						auto actual_request_ptr =
								get_actual_service_request_pointer<
											Result, payload_type >( message_ref );

						set_promise(
								actual_request_ptr->m_promise,
								[&] {
									return lambda(
											arg_maker::make_arg(
													actual_request_ptr->m_param ) );
								} );
					}
				else
					{
						lambda( arg_maker::make_arg( message_ref ) );
					}
			};

		return msg_type_and_handler_pair_t{
				arg_maker::traits_type::subscription_type_index(),
				method,
				arg_maker::traits_type::mutability() };
	}

/*!
 * \brief Helper template for creation of event handler with actual
 * argument.
 *
 * \since
 * v.5.5.14
 */
template< typename Agent, typename Method_Pointer >
typename std::enable_if<
		is_agent_method_pointer<Method_Pointer>::value,
		msg_type_and_handler_pair_t >::type
make_handler_with_arg_for_agent(
	Agent * agent,
	Method_Pointer pfn )
	{
		using pfn_traits = is_agent_method_pointer<Method_Pointer>;

		static_assert( std::is_same<Agent, typename pfn_traits::agent_type>::value,
				"Agent type must be the same" );

		using arg_maker = event_handler_arg_maker<
				typename so_5::details::lambda_traits::plain_argument_type<
						typename pfn_traits::argument_type>::type >;

		using payload_type = typename arg_maker::traits_type::payload_type;
		using result_type = typename pfn_traits::result_type;

		arg_maker::ensure_appropriate_type();

		auto method = [agent, pfn](
				invocation_type_t invocation_type,
				message_ref_t & message_ref)
			{
				if( invocation_type_t::service_request == invocation_type )
					{
						auto actual_request_ptr =
								get_actual_service_request_pointer<
											result_type, payload_type >( message_ref );

						set_promise(
								actual_request_ptr->m_promise,
								[&] {
									return (agent->*pfn)(
											arg_maker::make_arg(
													actual_request_ptr->m_param ) );
								} );
					}
				else
					{
						(agent->*pfn)( arg_maker::make_arg( message_ref ) );
					}
			};

		return msg_type_and_handler_pair_t{
				arg_maker::traits_type::subscription_type_index(),
				method,
				arg_maker::traits_type::mutability() };
	}

/*!
 * \brief Helper template for creation of event handler without actual
 * argument.
 *
 * \note This helper must be used only if Sig is derived from signal_t.
 *
 * \since
 * v.5.5.14
 */
template< typename Lambda, typename Result, typename Sig >
msg_type_and_handler_pair_t
make_handler_without_arg( Lambda && lambda )
	{
		ensure_signal< Sig >();

		auto method = [lambda](
				invocation_type_t invocation_type,
				message_ref_t & message_ref) mutable
			{
				if( invocation_type_t::service_request == invocation_type )
					{
						auto actual_request_ptr =
								get_actual_service_request_pointer<
											Result, Sig >(
										message_ref );

						set_promise(
								actual_request_ptr->m_promise,
								[&] { return lambda(); } );
					}
				else
					{
						lambda();
					}
			};

		return msg_type_and_handler_pair_t{
				message_payload_type< Sig >::subscription_type_index(),
				method,
				message_payload_type< Sig >::mutability() };
	}

/*!
 * \brief Ensure that mutability of message is compatible with
 * mutability of target mbox.
 *
 * \since
 * v.5.5.21
 */
inline void
ensure_handler_can_be_used_with_mbox(
	const ::so_5::details::msg_type_and_handler_pair_t & handler,
	const ::so_5::mbox_t & target_mbox )
{
	if( message_mutability_t::mutable_message == handler.m_mutability &&
			mbox_type_t::multi_producer_multi_consumer == target_mbox->type() )
		SO_5_THROW_EXCEPTION( rc_subscription_to_mutable_msg_from_mpmc_mbox,
				std::string( "subscription to mutable message from MPMC mbox "
						"is disabled, msg_type=" )
				+ handler.m_msg_type.name() );
}

} /* namespace event_subscription_helpers */

} /* namespace details */

//
// handler
//
/*!
 * \brief A function for creation event handler.
 *
 * \note Must be used for the case when message is an ordinary message.
 *
 * \note This function is intended to be used only by SObjectizer itself.
 *
 * \since
 * v.5.5.13
 */
template< class Lambda >
details::msg_type_and_handler_pair_t
handler( Lambda && lambda )
	{
		using namespace so_5::details::lambda_traits;
		using namespace so_5::details::event_subscription_helpers;

		typedef traits< typename std::decay< Lambda >::type > Traits;
		typedef typename Traits::result_type Result;
		typedef typename Traits::argument_type Message;
		typedef typename Traits::pass_by_type Transformed_Lambda;

		return make_handler_with_arg< Transformed_Lambda, Result, Message >(
				std::forward< Lambda >(lambda) );
	}

//
// handler
//
/*!
 * \brief A function for creation event handler.
 *
 * \note Must be used for the case when message is a signal.
 *
 * \note This function is intended to be used only by SObjectizer itself.
 *
 * \since
 * v.5.5.13
 */
template< class Signal, class Lambda >
details::msg_type_and_handler_pair_t
handler( Lambda && lambda )
	{
		using namespace so_5::details::lambda_traits;
		using namespace so_5::details::event_subscription_helpers;

		ensure_signal< Signal >();

		typedef traits< typename std::decay< Lambda >::type > Traits;
		typedef typename Traits::result_type Result;

		return make_handler_without_arg< Lambda, Result, Signal >(
				std::forward< Lambda >(lambda) );
	}

//
// preprocess_agent_event_handler
//
/*!
 * \brief Do preprocessing and some verification of event handler
 * and return msg_type_and_handler_pair for it.
 *
 * This overload is intended to be used for pointers to members.
 *
 * \note
 * Throws is handler can't be used with this type of mbox
 * (for example: handler is for mutable message but mbox is
 * Multi-Consumer one).
 *
 * \since
 * v.5.5.21
 */
template< typename Method_Pointer >
typename std::enable_if<
		details::is_agent_method_pointer<Method_Pointer>::value,
		details::msg_type_and_handler_pair_t >::type
preprocess_agent_event_handler(
	const mbox_t & mbox,
	agent_t & agent,
	Method_Pointer pfn )
	{
		using namespace details::event_subscription_helpers;

		using agent_type =
				typename details::is_agent_method_pointer<Method_Pointer>::agent_type;

		// Agent must have right type.
		auto cast_result = get_actual_agent_pointer< agent_type >( agent );

		const auto ev = make_handler_with_arg_for_agent( cast_result, pfn );

		ensure_handler_can_be_used_with_mbox( ev, mbox );

		return ev;
	}

//
// preprocess_agent_event_handler
//
/*!
 * \brief Do preprocessing and some verification of event handler
 * and return msg_type_and_handler_pair for it.
 *
 * This overload is intended to be used for lambdas or functional objects.
 *
 * \attention
 * Only lambda functions or functional objects in the following format
 * are supported:
 * \code
 * ret_type (message_type);
 * ret_type (const message_type &);
 * ret_type (mhood_t<message_type>);
 * \endcode
 *
 * \since
 * v.5.5.21
 */
template< typename Lambda >
typename std::enable_if<
		details::lambda_traits::is_lambda<Lambda>::value,
		details::msg_type_and_handler_pair_t >::type
preprocess_agent_event_handler(
	const mbox_t & mbox,
	agent_t & /*agent*/,
	Lambda && lambda )
	{
		using namespace details::event_subscription_helpers;

		const auto ev = handler( std::forward<Lambda>(lambda) );

		ensure_handler_can_be_used_with_mbox( ev, mbox );

		return ev;
	}

namespace details {

//
// handlers_bunch_basics_t
//
/*!
 * \brief Basic part of handlers_bunch implementation.
 *
 * \note This part is not depends on template parameters.
 * \since
 * v.5.5.13
 */
struct handlers_bunch_basics_t
	{
		/*!
		 * \brief Preparation of message handlers vector.
		 *
		 * Message handlers vector must be ordered by msg_type.
		 * And must not contain two or more handlers for the same msg_type.
		 *
		 * Vector to be processed is defined by range [left, right).
		 *
		 * \throw so_5::exception_t if there are several handlers for
		 * the same message type.
		 */
		static inline void
		prepare_handlers(
			msg_type_and_handler_pair_t * left,
			msg_type_and_handler_pair_t * right )
			{
				std::sort( left, right );
				auto duplicate = std::adjacent_find( left, right );
				if( duplicate != right )
					SO_5_THROW_EXCEPTION( rc_several_handlers_for_one_message_type,
							std::string( "several handlers are defined for message; "
									"msg_type: " ) + duplicate->m_msg_type.name() );
			}

		/*!
		 * \brief Find and exec message handler.
		 *
		 * Message handlers vector to be used in handling of message
		 * is defined by range [left, right).
		 *
		 * \note The message handler vector [left, right) must be previously
		 * prepared by prepare_handlers().
		 *
		 * \retval true if handler has been found
		 * \retval false if handler has not been found.
		 */
		SO_5_FUNC
		static bool
		find_and_use_handler(
			const msg_type_and_handler_pair_t * left,
			const msg_type_and_handler_pair_t * right,
			const std::type_index & msg_type,
			message_ref_t & message,
			invocation_type_t invocation );
	};

//
// handlers_bunch
//
/*!
 * \brief Template class for storing bunch of message handlers.
 * \since
 * v.5.5.13
 */
template< std::size_t N >
class handlers_bunch_t : private handlers_bunch_basics_t
	{
		//! Vector of message handlers.
		/*!
		 * Will be ordered by msg_type after invoking prepare() method.
		 */
		msg_type_and_handler_pair_t m_handlers[ N ];

	public :
		handlers_bunch_t()
			{}

		//! Add another handler to the specified index.
		void
		add_handler(
			//! Index for new handler.
			std::size_t index,
			//! Message handler to be added.
			msg_type_and_handler_pair_t && handler )
			{
				m_handlers[ index ] = std::move(handler);
			}

		//! Prepare bunch to use with actual messages.
		/*!
		 * \note This method must be called only after all handlers are
		 * stored in m_handlers vector.
		 */
		void
		prepare()
			{
				prepare_handlers( m_handlers, m_handlers + N );
			}

		//! Find handler for a message and execute it.
		/*!
		 * \retval true if handler was found.
		 * \retval false if handler was not found.
		 */
		bool
		handle(
			//! Type of a message or signal.
			const std::type_index & msg_type,
			//! Message instance to be processed.
			message_ref_t & message,
			//! It is async message or service handler?
			invocation_type_t invocation ) const
			{
				return find_and_use_handler(
						m_handlers, m_handlers + N,
						msg_type,
						message,
						invocation );
			}
	};

/*!
 * \brief A specialization of handlers_bunch_t for the case when there
 * is no any handlers.
 * \since
 * v.5.5.13
 */
template<>
class handlers_bunch_t< 0 >
	{
	public :
		handlers_bunch_t()
			{}

		void
		prepare()
			{
				/* NOTHING TO DO! */
			}

		bool
		handle(
			const std::type_index & /*msg_type*/,
			message_ref_t & /*message*/,
			invocation_type_t /*invocation*/ ) const
			{
				return false;
			}
	};

//
// fill_handlers_bunch
//

template< typename Bunch >
void
fill_handlers_bunch( Bunch & bunch, std::size_t )
	{
		bunch.prepare();
	}

template< typename Bunch, typename... Others >
void
fill_handlers_bunch(
	//! What to fill.
	Bunch & bunch,
	//! An index for next handler.
	std::size_t index,
	//! Next handler to be inserted.
	msg_type_and_handler_pair_t && handler,
	//! All other handlers.
	Others &&... other_handlers )
	{
		bunch.add_handler( index, std::move(handler) );
		fill_handlers_bunch( bunch, index + 1,
				std::forward< Others >(other_handlers)... );
	}

template< typename Bunch, typename Lambda, typename... Others >
void
fill_handlers_bunch(
	//! What to fill.
	Bunch & bunch,
	//! An index for next handler.
	std::size_t index,
	//! Next handler to be inserted.
	Lambda && lambda,
	//! All other handlers.
	Others &&... other_handlers )
	{
		bunch.add_handler( index,
				handler( std::forward<Lambda>( lambda ) ) );
		fill_handlers_bunch( bunch, index + 1,
				std::forward< Others >(other_handlers)... );
	}

} /* namespace details */

} /* namespace so_5 */


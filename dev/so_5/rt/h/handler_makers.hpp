/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.13
 * \file
 * \brief Various stuff for creation of event handlers.
 */

#pragma once

#include <so_5/h/exception.hpp>
#include <so_5/h/ret_code.hpp>

#include <so_5/details/h/lambda_traits.hpp>

#include <so_5/rt/h/execution_demand.hpp>

#include <algorithm>

namespace so_5 {

namespace details {

/*!
 * \since v.5.3.0
 * \brief Various helpers for event subscription.
 */
namespace event_subscription_helpers
{

/*!
 * \brief Get actual agent pointer.
 *
 * \throw exception_t if dynamic_cast fails.
 */
template< class AGENT >
AGENT *
get_actual_agent_pointer( agent_t & agent )
{
	// Agent must have right type.
	AGENT * cast_result = dynamic_cast< AGENT * >( &agent );

	// Was conversion successful?
	if( nullptr == cast_result )
	{
		// No. Actual type of the agent is not convertible to the AGENT.
		SO_5_THROW_EXCEPTION(
			rc_agent_incompatible_type_conversion,
			std::string( "Unable convert agent to type: " ) +
				typeid(AGENT).name() );
	}

	return cast_result;
}

/*!
 * \brief Get actual msg_service_request pointer.
 *
 * \throw exception_t if dynamic_cast fails.
 */
template< class RESULT, class MESSAGE >
msg_service_request_t<
		RESULT,
		typename message_payload_type< MESSAGE >::envelope_type > *
get_actual_service_request_pointer(
	const message_ref_t & message_ref )
{
	using actual_request_msg_t =
			msg_service_request_t<
					RESULT,
					typename message_payload_type< MESSAGE >::envelope_type >;

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

} /* namespace event_subscription_helpers */

/*!
 * \since v.5.3.0
 * \brief Internal namespace for details of agent method invocation implementation.
 */
namespace promise_result_setting_details
{

using namespace so_5::details::lambda_traits;

template< class RESULT >
struct result_setter_t
	{
		template< class AGENT, class PARAM >
		void
		call_old_format_event_and_set_result(
			std::promise< RESULT > & to,
			AGENT * a,
			RESULT (AGENT::*pfn)( const event_data_t< PARAM > & ),
			const event_data_t< PARAM > & evt )
			{
				to.set_value( (a->*pfn)( evt ) );
			}

		template< class AGENT, class METHOD, class PARAM >
		void
		call_new_format_event_and_set_result(
			std::promise< RESULT > & to,
			AGENT * a,
			METHOD pfn,
			const PARAM & msg )
			{
				to.set_value( (a->*pfn)( msg ) );
			}

		template< class AGENT >
		void
		call_new_format_signal_and_set_result(
			std::promise< RESULT > & to,
			AGENT * a,
			RESULT (AGENT::*pfn)() )
			{
				to.set_value( (a->*pfn)() );
			}

		template< class LAMBDA, class PARAM >
		void
		call_event_lambda_and_set_result(
			std::promise< RESULT > & to,
			LAMBDA l,
			const PARAM & msg )
			{
				to.set_value( traits< LAMBDA >::call_with_arg( l, msg ) );
			}

		template< class LAMBDA >
		void
		call_signal_lambda_and_set_result(
			std::promise< RESULT > & to,
			LAMBDA l )
			{
				to.set_value( traits< LAMBDA >::call_without_arg( l ) );
			}
	};

template<>
struct result_setter_t< void >
	{
		template< class AGENT, class PARAM >
		void
		call_old_format_event_and_set_result(
			std::promise< void > & to,
			AGENT * a,
			void (AGENT::*pfn)( const event_data_t< PARAM > & ),
			const event_data_t< PARAM > & evt )
			{
				(a->*pfn)( evt );
				to.set_value();
			}

		template< class AGENT, class METHOD, class PARAM >
		void
		call_new_format_event_and_set_result(
			std::promise< void > & to,
			AGENT * a,
			METHOD pfn,
			const PARAM & msg )
			{
				(a->*pfn)( msg );
				to.set_value();
			}

		template< class AGENT >
		void
		call_new_format_signal_and_set_result(
			std::promise< void > & to,
			AGENT * a,
			void (AGENT::*pfn)() )
			{
				(a->*pfn)();
				to.set_value();
			}

		template< class LAMBDA, class PARAM >
		void
		call_event_lambda_and_set_result(
			std::promise< void > & to,
			LAMBDA l,
			const PARAM & msg )
			{
				traits< LAMBDA >::call_with_arg( l, msg );
				to.set_value();
			}

		template< class LAMBDA >
		void
		call_signal_lambda_and_set_result(
			std::promise< void > & to,
			LAMBDA l )
			{
				traits< LAMBDA >::call_without_arg( l );
				to.set_value();
			}
	};

} /* namespace promise_result_setting_details */


} /* namespace details */

//
// handler
//
/*!
 * \since v.5.5.13
 * \brief A function for creation event handler.
 *
 * \note Must be used for the case when message is an ordinary message.
 *
 * \note This function is intended to be used only by SObjectizer itself.
 */
template< class LAMBDA >
details::msg_type_and_handler_pair_t
handler( LAMBDA && lambda )
	{
		using namespace so_5::details;
		using namespace so_5::details::lambda_traits;
		using namespace so_5::details::event_subscription_helpers;
		using namespace so_5::details::promise_result_setting_details;

		typedef traits< typename std::decay< LAMBDA >::type > TRAITS;
		typedef typename TRAITS::result_type RESULT;
		typedef typename TRAITS::argument_type MESSAGE;

		ensure_not_signal< MESSAGE >();

		auto method = [lambda](
				invocation_type_t invocation_type,
				message_ref_t & message_ref)
			{
				if( invocation_type_t::service_request == invocation_type )
					{
						auto actual_request_ptr =
								get_actual_service_request_pointer< RESULT, MESSAGE >(
										message_ref );

						auto msg = message_payload_type< MESSAGE >::extract_payload_ptr(
								actual_request_ptr->m_param );
						ensure_message_with_actual_data( msg );

						// All exceptions will be processed in service_handler_on_message.
						result_setter_t< RESULT >().call_event_lambda_and_set_result(
								actual_request_ptr->m_promise,
								lambda,
								*msg );
					}
				else
					{
						auto msg = message_payload_type< MESSAGE >::extract_payload_ptr(
								message_ref );
						ensure_message_with_actual_data( msg );

						TRAITS::call_with_arg( lambda, *msg );
					}
			};

		return msg_type_and_handler_pair_t{
				message_payload_type< MESSAGE >::payload_type_index(),
				method };
	}

//
// handler
//
/*!
 * \since v.5.5.13
 * \brief A function for creation event handler.
 *
 * \note Must be used for the case when message is a signal.
 *
 * \note This function is intended to be used only by SObjectizer itself.
 */
template< class SIGNAL, class LAMBDA >
details::msg_type_and_handler_pair_t
handler( LAMBDA && lambda )
	{
		using namespace so_5::details;
		using namespace so_5::details::lambda_traits;
		using namespace so_5::details::event_subscription_helpers;
		using namespace so_5::details::promise_result_setting_details;

		ensure_signal< SIGNAL >();

		typedef traits< typename std::decay< LAMBDA >::type > TRAITS;
		typedef typename TRAITS::result_type RESULT;

		auto method = [lambda](
				invocation_type_t invocation_type,
				message_ref_t & message_ref)
			{
				if( invocation_type_t::service_request == invocation_type )
					{
						auto actual_request_ptr =
								get_actual_service_request_pointer< RESULT, SIGNAL >(
										message_ref );

						// All exceptions will be processed in service_handler_on_message.
						result_setter_t< RESULT >().call_signal_lambda_and_set_result(
								actual_request_ptr->m_promise,
								lambda );
					}
				else
					{
						TRAITS::call_without_arg( lambda );
					}
			};

		return msg_type_and_handler_pair_t{
				message_payload_type< SIGNAL >::payload_type_index(),
				method };
	}

namespace details {

//
// handlers_bunch_basics_t
//
/*!
 * \since v.5.5.13
 * \brief Basic part of handlers_bunch implementation.
 *
 * \note This part is not depends on template parameters.
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
		static inline bool
		find_and_use_handler(
			const msg_type_and_handler_pair_t * left,
			const msg_type_and_handler_pair_t * right,
			const std::type_index & msg_type,
			message_ref_t & message,
			invocation_type_t invocation )
			{
				bool ret_value = false;

				msg_type_and_handler_pair_t key{ msg_type };
				auto it = std::lower_bound( left, right, key );
				if( it != right && it->m_msg_type == key.m_msg_type )
					{
						// Handler is found and must be called.
						ret_value = true;
						if( invocation_type_t::event == invocation )
							// This is a async message.
							// Simple call is enough.
							it->m_handler( invocation, message );
						else if( invocation_type_t::service_request == invocation )
							// Invocation should be done in a special wrapper.
							msg_service_request_base_t::dispatch_wrapper(
									message,
									[&it, &message, invocation] {
										it->m_handler( invocation, message );
									} );
					}

				return ret_value;
			}
	};

//
// handlers_bunch
//
/*!
 * \since v.5.5.13
 * \brief Template class for storing bunch of message handlers.
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
 * \since v.5.5.13
 * \brief A specialization of handlers_bunch_t for the case when there
 * is no any handlers.
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

template< typename BUNCH >
void
fill_handlers_bunch( BUNCH & bunch, std::size_t )
	{
		bunch.prepare();
	}

template< typename BUNCH, typename... OTHERS >
void
fill_handlers_bunch(
	//! What to fill.
	BUNCH & bunch,
	//! An index for next handler.
	std::size_t index,
	//! Next handler to be inserted.
	msg_type_and_handler_pair_t && handler,
	//! All other handlers.
	OTHERS &&... other_handlers )
	{
		bunch.add_handler( index, std::move(handler) );
		fill_handlers_bunch( bunch, index + 1,
				std::forward< OTHERS >(other_handlers)... );
	}

template< typename BUNCH, typename LAMBDA, typename... OTHERS >
void
fill_handlers_bunch(
	//! What to fill.
	BUNCH & bunch,
	//! An index for next handler.
	std::size_t index,
	//! Next handler to be inserted.
	LAMBDA && lambda,
	//! All other handlers.
	OTHERS &&... other_handlers )
	{
		bunch.add_handler( index,
				handler( std::forward<LAMBDA>( lambda ) ) );
		fill_handlers_bunch( bunch, index + 1,
				std::forward< OTHERS >(other_handlers)... );
	}

} /* namespace details */

} /* namespace so_5 */


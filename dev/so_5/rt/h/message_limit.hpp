/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Public part of message limit implementation.
 */

#pragma once

#include <so_5/rt/h/message.hpp>
#include <so_5/rt/h/mbox.hpp>

#include <so_5/details/h/lambda_traits.hpp>
#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <so_5/h/declspec.hpp>

#include <functional>
#include <typeindex>
#include <atomic>
#include <vector>

namespace so_5
{

namespace message_limit
{

//
// description_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A description of one message limit.
 */
struct description_t
	{
		//! Type of message.
		std::type_index m_msg_type;

		//! Max count of waiting messages.
		unsigned int m_limit;	

		//! Reaction to overload.
		action_t m_action;

		//! Initializing constructor.
		description_t(
			std::type_index msg_type,
			unsigned int limit,
			action_t action )
			:	m_msg_type( std::move( msg_type ) )
			,	m_limit( limit )
			,	m_action( std::move( action ) )
			{}
	};

//
// description_container_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Type of container for holding message limit descriptions.
 */
using description_container_t = std::vector< description_t >;

namespace impl
{

/*!
 * \since
 * v.5.5.9
 *
 * \brief Actual implementation of drop message reaction.
 */
SO_5_FUNC
void
drop_message_reaction( const overlimit_context_t & ctx );

} /* namespace impl */

//
// drop_indicator_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Message limit with reaction 'drop new message'.
 */
template< class M >
struct drop_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		//! Initializing constructor.
		drop_indicator_t( unsigned int limit )
			:	m_limit( limit )
			{}
	};

//
// accept_one_indicator
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for accepting drop_indicator and storing
 * the corresponding description into the limits container.
 */
template< class M >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of drop_indicator.
	const drop_indicator_t< M > & indicator )
	{
		to.emplace_back( message_payload_type< M >::subscription_type_index(),
				indicator.m_limit,
				&impl::drop_message_reaction );
	}

namespace impl
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief Actual implementation of abort application reaction.
 */
SO_5_FUNC
void
abort_app_reaction( const overlimit_context_t & ctx );

} /* namespace impl */

//
// abort_app_indicator_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Message limit with reaction 'abort the application'.
 */
template< class M >
struct abort_app_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		//! Initializing constructor.
		abort_app_indicator_t( unsigned int limit )
			:	m_limit( limit )
			{}
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for accepting abort_app_indicator and storing
 * the corresponding description into the limits container.
 */
template< class M >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of abort_app_indicator to store.
	const abort_app_indicator_t< M > & indicator )
	{
		to.emplace_back( message_payload_type< M >::subscription_type_index(),
				indicator.m_limit,
				[]( const overlimit_context_t & ctx ) {
					impl::abort_app_reaction( ctx );
				} );
	}

/*!
 * \since
 * v.5.5.8
 *
 * \brief Message limit with reaction 'abort the application' and the
 * possibility to call additional lambda before aborting the app.
 */
template< typename M, typename L >
struct log_then_abort_app_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		//! Lambda for some last actions.
		const L m_lambda;

		//! Initializing constructor.
		log_then_abort_app_indicator_t(
			unsigned int limit,
			L lambda )
			:	m_limit( limit )
			,	m_lambda( std::move( lambda ) )
			{}
	};

namespace impl
{

/*!
 * \since
 * v.5.5.8
 *
 * \brief Helper class for calling pre-abort action.
 */
template< bool IS_MESSAGE, typename M, typename L >
struct call_pre_abort_action_impl
	{
		static void
		call( const overlimit_context_t & ctx, L action )
			{
				const auto & m = message_payload_type< M >::payload_reference(
						*ctx.m_message );
				action( ctx.m_receiver, m );
			}
	};

/*!
 * \since
 * v.5.5.8
 *
 * \brief Specialization for the case of signal.
 */
template< typename M, typename L >
struct call_pre_abort_action_impl< false, M, L >
	{
		static void
		call( const overlimit_context_t & ctx, L action )
			{
				action( ctx.m_receiver );
			}
	};

/*!
 * \since
 * v.5.5.8
 *
 * \brief Helper class for calling pre-abort action.
 *
 * \tparam M type of message or signal
 * \tparam L lambda function to be called before aborting app.
 *
 * This lambda function must no throw exceptions.
 * For the case when M is a message type L should have prototype:
 * \code
	void lambda(const agent_t &, const M &);
 * \endcode
 * For the case when M is a signal type L should have prototype:
 * \code
	void lambda(const agent_t &);
 * \endcode
 */
template< typename M, typename L >
struct call_pre_abort_action
	:	public call_pre_abort_action_impl< !is_signal< M >::value, M, L >
{};

} /* namespace impl */

/*!
 * \since
 * v.5.5.8
 *
 * \brief Helper function for accepting log_then_abort_app_indicator and
 * storing the corresponding description into the limits container.
 */
template< class M, class L >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of abort_app_indicator to store.
	const log_then_abort_app_indicator_t< M, L > & indicator )
	{
		auto lambda = indicator.m_lambda;

		to.emplace_back( message_payload_type< M >::subscription_type_index(),
				indicator.m_limit,
				[lambda]( const overlimit_context_t & ctx ) {
					so_5::details::invoke_noexcept_code( [&] {
							impl::call_pre_abort_action< M, L >::call( ctx, lambda );
						} );

					impl::abort_app_reaction( ctx );
				} );
	}

namespace impl
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief Actual implementation of redirect message reaction.
 */
SO_5_FUNC
void
redirect_reaction(
	//! Context on which overlimit must be handled.
	const overlimit_context_t & ctx,
	//! Destination for message redirection.
	const mbox_t & to );

} /* namespace impl */

//
// redirect_indicator_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Indication that a message must be redirected on overlimit.
 *
 * \tparam MSG Message type of message/signal to be redirected.
 * \tparam LAMBDA Type of lambda- or functional object which returns
 * actual mbox for redirection.
 */
template< typename MSG, typename LAMBDA >
struct redirect_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		//! A lambda/functional object which returns mbox for redirection.
		LAMBDA m_destination_getter;

		//! Initializing constructor.
		redirect_indicator_t(
			unsigned int limit,
			LAMBDA destination_getter )
			:	m_limit( limit )
			,	m_destination_getter( std::move( destination_getter ) )
			{}
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for accepting redirect_indicator and storing
 * the corresponding description into the limits container.
 */
template< typename MSG, typename LAMBDA >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of redirect_indicator to be stored.
	redirect_indicator_t< MSG, LAMBDA > indicator )
	{
		LAMBDA dest_getter = std::move( indicator.m_destination_getter );
		to.emplace_back( message_payload_type< MSG >::subscription_type_index(),
				indicator.m_limit,
				[dest_getter]( const overlimit_context_t & ctx ) {
					impl::redirect_reaction( ctx, dest_getter() );
				} );
	}

namespace impl
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief Checks that service request will not be transformed.
 */
SO_5_FUNC
void
ensure_event_transform_reaction(
	//! Context on which overlimit must be handled.
	const overlimit_context_t & ctx );

/*!
 * \since
 * v.5.5.4
 *
 * \brief Actual implementation of transform reaction.
 */
SO_5_FUNC
void
transform_reaction(
	//! Context on which overlimit must be handled.
	const overlimit_context_t & ctx,
	//! Destination for new message.
	const mbox_t & to,
	//! Type of new message.
	const std::type_index & msg_type,
	//! An instance of new message.
	const message_ref_t & message );

//
// transformed_message_maker
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A helper function to call appropriate constructor of
 * resulting type in dependence of message or signal.
 *
 * \tparam is_signal true if MSG is a signal.
 * \tparam RESULT Result type object of that will be created by make() method.
 * \tparam MSG Type of message to be instantiated.
 * \tparam ARGS Type of parameters to message constructor.
 */
template< bool is_signal, typename RESULT, typename MSG, typename... ARGS >
struct transformed_message_maker
	{
		//! Create a message instance.
		static RESULT
		make( mbox_t mbox, ARGS &&... args )
			{
				return RESULT( std::move( mbox ),
						so_5::details::make_message_instance< MSG >(
								std::forward<ARGS>( args )... ) );
			}
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief A specialization of signal.
 */
template< typename RESULT, typename MSG >
struct transformed_message_maker< true, RESULT, MSG >
	{
		static RESULT
		make( mbox_t mbox )
			{
				return RESULT( std::move( mbox ) );
			}
	};

} /* namespace impl */

//
// transformed_message_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A result of message transformation.
 *
 * \tparam MSG Type of result (transformed) message.
 */
template< typename MSG >
class transformed_message_t
	{
	public :
		//! Initializing constructor for the case when MSG is a message type.
		transformed_message_t(
			//! Message box to which transformed message to be sent.
			mbox_t mbox,
			//! New message instance.
			std::unique_ptr< typename message_payload_type< MSG >::envelope_type > msg )
			:	m_mbox( std::move( mbox ) )
			{
				ensure_message_with_actual_data( msg.get() );

				m_message = message_ref_t( msg.release() );
			}
		//! Initializing constructor for the case when MSG is a signal type.
		transformed_message_t(
			//! Message box to which signal to be sent.
			mbox_t mbox )
			:	m_mbox( std::move( mbox ) )
			{
				ensure_signal< MSG >();
			}

		//! Destination message box.
		const mbox_t &
		mbox() const { return m_mbox; }

		//! Type of the transformed message.
		std::type_index
		msg_type() const { return message_payload_type< MSG >::subscription_type_index(); }

		//! Instance of transformed message.
		/*!
		 * \note Will be nullptr for signal.
		 */
		const message_ref_t &
		message() const { return m_message; }

		//! A helper method for transformed_message construction.
		template< typename... ARGS >
		static transformed_message_t< MSG >
		make( mbox_t mbox, ARGS &&... args )
			{
				return impl::transformed_message_maker<
							is_signal< MSG >::value,
							transformed_message_t,
							MSG,
							ARGS...
						>::make(
								std::move( mbox ),
								std::forward<ARGS>( args )... );
			}

	private :
		//! Destination message box.
		mbox_t m_mbox;

		//! Instance of transformed message.
		/*!
		 * \note Will be nullptr for signal.
		 */
		message_ref_t m_message;
	};

//
// transform_indicator_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief An indicator of transform reaction on message overlimit.
 *
 * \tparam SOURCE Type of message to be transformed.
 */
template< typename SOURCE >
struct transform_indicator_t
	{
		//! Limit value.
		unsigned int m_limit;

		//! Reaction.
		action_t m_action;

		//! Initializing constructor.
		transform_indicator_t(
			unsigned int limit,
			action_t action )
			:	m_limit( limit )
			,	m_action( std::move( action ) )
			{}
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for accepting transform_indicator and storing
 * the corresponding description into the limits container.
 */
template< class M >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of transform_indicator to be stored.
	transform_indicator_t< M > indicator )
	{
		to.emplace_back( message_payload_type< M >::subscription_type_index(),
				indicator.m_limit,
				std::move( indicator.m_action ) );
	}

//
// accept_indicators
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper function for constructing limits description from
 * a series of limit indicators.
 */
template< typename I, typename... ARGS >
void
accept_indicators(
	description_container_t & to,
	I && indicator,
	ARGS &&... others )
	{
		accept_one_indicator( to, std::forward< I >( indicator ) );

		accept_indicators( to, std::forward< ARGS >( others )... );
	}

inline void
accept_indicators(
	description_container_t & )
	{
	}

//
// message_limit_methods_mixin_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief A mixin with message limit definition methods.
 */
struct message_limit_methods_mixin_t
	{
		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating drop_indicator.
		 */
		template< typename MSG >
		static drop_indicator_t< MSG >
		limit_then_drop( unsigned int limit )
			{
				return drop_indicator_t< MSG >( limit );
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating abort_app_indicator.
		 */
		template< typename MSG >
		static abort_app_indicator_t< MSG >
		limit_then_abort( unsigned int limit )
			{
				return abort_app_indicator_t< MSG >( limit );
			}

		/*!
		 * \since
		 * v.5.5.8
		 *
		 * \brief A helper function for creating log_then_abort_app_indicator.
		 *
		 * \tparam M type of message or signal
		 * \tparam L lambda function to be called before aborting app.
		 *
		 * This lambda function must no throw exceptions.
		 * For the case when M is a message type L should have prototype:
		 * \code
			void lambda(const agent_t &, const M &);
		 * \endcode
		 * For the case when M is a signal type L should have prototype:
		 * \code
			void lambda(const agent_t &);
		 * \endcode
		 */
		template< typename M, typename L >
		static log_then_abort_app_indicator_t< M, L >
		limit_then_abort( unsigned int limit, L lambda )
			{
				return log_then_abort_app_indicator_t< M, L >{
						limit, std::move(lambda) };
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating redirect_indicator.
		 */
		template< typename MSG, typename LAMBDA >
		static redirect_indicator_t< MSG, LAMBDA >
		limit_then_redirect(
			unsigned int limit,
			LAMBDA dest_getter )
			{
				return redirect_indicator_t< MSG, LAMBDA >(
						limit,
						std::move( dest_getter ) );
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating transform_indicator.
		 *
		 * \note Must be used for message transformation, signals cannot be
		 * transformed. Type of message is detected automatically from the type
		 * of transformation lambda argument.
		 *
		 * \par Usage example:
		 * \code
			class a_request_processor_t : public so_5::agent_t
			{
			public :
				a_request_processor_t( context_t ctx )
					:	so_5::agent_t( ctx 
							// Limit count of requests in the queue.
							// If queue is full then request must be transformed
							// to negative reply.
							+ limit_then_transform( 3,
								[]( const request & evt ) {
									return make_transformed< reply >(
											evt.m_reply_to, evt.m_id, false );
								} ) )
					{...}
				...
			};
		 * \endcode
		 */
		template<
				typename LAMBDA,
				typename ARG = typename so_5::details::lambda_traits::
						argument_type_if_lambda< LAMBDA >::type >
		static transform_indicator_t< ARG >
		limit_then_transform(
			unsigned int limit,
			LAMBDA transformator )
			{
				ensure_not_signal< ARG >();

				action_t act = [transformator]( const overlimit_context_t & ctx ) {
						// Service request cannot be transformed.
						// So ensure that is event, not service request.
						impl::ensure_event_transform_reaction( ctx );

						const auto & msg =
								message_payload_type< ARG >::payload_reference(
										*ctx.m_message.get() );
						auto r = transformator( msg );
						impl::transform_reaction(
								ctx, r.mbox(), r.msg_type(), r.message() );
					};

				return transform_indicator_t< ARG >{ limit, std::move( act ) };
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating transform_indicator.
		 *
		 * Must be used for signal transformation. Type of signal must be
		 * explicitely specified.
		 */
		template< typename SOURCE, typename LAMBDA >
		static transform_indicator_t< SOURCE >
		limit_then_transform(
			unsigned int limit,
			LAMBDA transformator )
			{
				ensure_signal< SOURCE >();

				action_t act = [transformator]( const overlimit_context_t & ctx ) {
						// Service request cannot be transformed.
						// So ensure that is event, not service request.
						impl::ensure_event_transform_reaction( ctx );

						auto r = transformator();
						impl::transform_reaction(
								ctx, r.mbox(), r.msg_type(), r.message() );
					};

				return transform_indicator_t< SOURCE >{ limit, std::move( act ) };
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Helper method for creating message transformation result.
		 *
		 * \par Usage examples:
			\code
			class request_acceptor : public so_5::agent_t
				{
				public :
					request_acceptor(
						context_t ctx,
						so_5::mbox_t interaction_mbox,
						request_scheduling_data & data )
						:	so_5::agent_t( ctx
								// If there are to many pending requests then
								// new requests must be rejected.
								+ limit_then_transform( 10,
									[this]( const generation_request & req ) {
										return make_transformed< generation_rejected >(
												m_interaction_mbox,
												req.m_id );
										} ) )
						,	m_interaction_mbox( std::move( interaction_mbox ) )
						,	m_data( data )
						{}
				...
				};
			\endcode
		 */
		template< typename MSG, typename... ARGS >
		static transformed_message_t< MSG >
		make_transformed( mbox_t mbox, ARGS &&... args )
			{
				return transformed_message_t< MSG >::make(
						std::move( mbox ),
						std::forward<ARGS>( args )... );
			}
	};

namespace impl
{

// Forward declaration.
class info_storage_t;

} /* namespace impl */

} /* namespace message_limit */

namespace rt
{

namespace message_limit
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::message_limit::description_t instead.
 */
using description_t = so_5::message_limit::description_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::message_limit::description_container_t instead.
 */
using description_container_t = so_5::message_limit::description_container_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::message_limit::transformed_message_t instead.
 */
template< typename MSG >
using transformed_message_t = so_5::message_limit::transformed_message_t< MSG >;

} /* namespace message_limit */

} /* namespace rt */

} /* namespace so_5 */


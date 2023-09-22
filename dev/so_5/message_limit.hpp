/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Public part of message limit implementation.
 *
 * \since v.5.5.4
 */

#pragma once

#include <so_5/message.hpp>
#include <so_5/mbox.hpp>
#include <so_5/transformed_message.hpp>
#include <so_5/enveloped_msg.hpp>

#include <so_5/details/lambda_traits.hpp>
#include <so_5/details/invoke_noexcept_code.hpp>

#include <so_5/optional.hpp>

#include <so_5/declspec.hpp>

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
template< bool is_message, typename M, typename L >
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
 * \tparam Msg Message type of message/signal to be redirected.
 * \tparam Lambda Type of lambda- or functional object which returns
 * actual mbox for redirection.
 */
template< typename Msg, typename Lambda >
struct redirect_indicator_t
	{
		//! Max count of waiting messages.
		const unsigned int m_limit;

		//! A lambda/functional object which returns mbox for redirection.
		Lambda m_destination_getter;

		//! Initializing constructor.
		redirect_indicator_t(
			unsigned int limit,
			Lambda destination_getter )
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
template< typename Msg, typename Lambda >
void
accept_one_indicator(
	//! Container for storing new description to.
	description_container_t & to,
	//! An instance of redirect_indicator to be stored.
	redirect_indicator_t< Msg, Lambda > indicator )
	{
		Lambda dest_getter = std::move( indicator.m_destination_getter );
		to.emplace_back( message_payload_type< Msg >::subscription_type_index(),
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


} /* namespace impl */

//
// transform_indicator_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief An indicator of transform reaction on message overlimit.
 *
 * \tparam Source Type of message to be transformed.
 */
template< typename Source >
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
template< typename I, typename... Args >
void
accept_indicators(
	description_container_t & to,
	I && indicator,
	Args &&... others )
	{
		accept_one_indicator( to, std::forward< I >( indicator ) );

		accept_indicators( to, std::forward< Args >( others )... );
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
		using any_unspecified_message =
				::so_5::message_limit::any_unspecified_message;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating drop_indicator.
		 */
		template< typename Msg >
		[[nodiscard]]
		static drop_indicator_t< Msg >
		limit_then_drop( unsigned int limit )
			{
				return drop_indicator_t< Msg >( limit );
			}

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief A helper function for creating abort_app_indicator.
		 */
		template< typename Msg >
		[[nodiscard]]
		static abort_app_indicator_t< Msg >
		limit_then_abort( unsigned int limit )
			{
				return abort_app_indicator_t< Msg >( limit );
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
		[[nodiscard]]
		static log_then_abort_app_indicator_t< M, L >
		limit_then_abort( unsigned int limit, L lambda )
			{
				return log_then_abort_app_indicator_t< M, L >{
						limit, std::move(lambda) };
			}

		/*!
		 * \brief A helper function for creating redirect_indicator.
		 *
		 * The \a dest_getter should be a functor (or just function) with the
		 * following prototype:
		 * \code
		 * so_5::mbox_t dest_getter();
		 * \endcode
		 *
		 * \par Usage example:
		 * \code
		 * class main_message_handler : public so_5::agent_t {
		 * 	// Destination for message for an overload case.
		 * 	// NOTE: this mbox isn't known in the constructor.
		 * 	so_5::mbox_t overload_dest_;
		 * 	...
		 * public:
		 * 	main_message_handler(context_t ctx)
		 * 		:	so_5::agent_t{ ctx
		 * 				+ limit_then_redirect<data_message>( 10,
		 * 					[this]() { return overload_dest_; } )
		 * 			}
		 * 	{}
		 *
		 * 	// Setter for overload_dest.
		 * 	void set_overload_dest(const so_5::mbox_t & dest) {
		 * 		overload_dest_ = dest;
		 * 	}
		 * 	...
		 * };
		 * \endcode
		 *
		 * \since v.5.5.4
		 */
		template< typename Msg, typename Lambda >
		[[nodiscard]]
		static redirect_indicator_t< Msg, Lambda >
		limit_then_redirect(
			unsigned int limit,
			Lambda dest_getter )
			{
				return redirect_indicator_t< Msg, Lambda >(
						limit,
						std::move( dest_getter ) );
			}

		/*!
		 * \brief A helper function for creating redirect_indicator.
		 *
		 * This helper can be used if the target mbox is already known:
		 * \code
		 * class processor final : public so_5::agent_t
		 * {
		 * public:
		 * 	processor(context_t ctx, so_5::mbox_t overload_handler)
		 * 		:	so_5::agent_t{ ctx
		 * 				+ limit_then_redirect< some_message >( 10u, overload_handler )
		 * 				+ limit_then_redirect< another_message >( 20u, overload_handler )
		 * 			}
		 * 	{...}
		 * 
		 * ...
		 * };
		 * \endcode
		 *
		 * \since
		 * v.5.7.1
		 */
		template< typename Msg >
		[[nodiscard]]
		static auto
		limit_then_redirect(
			unsigned int limit,
			mbox_t destination )
			{
				return limit_then_redirect< Msg >(
						limit,
						[dest = std::move(destination)]() -> const so_5::mbox_t & {
							return dest;
						} );
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
				typename Lambda,
				typename Arg = typename so_5::details::lambda_traits::
						argument_type_if_lambda< Lambda >::type >
		[[nodiscard]]
		static transform_indicator_t< Arg >
		limit_then_transform(
			unsigned int limit,
			Lambda transformator )
			{
				ensure_not_signal< Arg >();
				static_assert( !std::is_same_v< Arg, any_unspecified_message >,
						"limit_then_transform can't be used for any_unspecified_message" );

				action_t act = [transformator]( const overlimit_context_t & ctx ) {
						const auto actual_transform =
								[&]( const message_ref_t & msg_to_transform ) {
									const auto & msg =
											message_payload_type< Arg >::payload_reference(
													*msg_to_transform.get() );
									auto r = transformator( msg );
									impl::transform_reaction(
											ctx, r.mbox(), r.msg_type(), r.message() );
								};

						// Envelopes should be handled a special way.
						// Payload must be extrected and checked for presence.
						if( message_t::kind_t::enveloped_msg == message_kind(
									ctx.m_message ) )
							{
								const auto opt_payload = ::so_5::enveloped_msg::
										extract_payload_for_message_transformation(
												ctx.m_message );

								// Payload can be optional. Se we will perform
								// transformation only if payload is present.
								if( opt_payload )
									{
										actual_transform( opt_payload->message() );
									}
							}
						else
							{
								actual_transform( ctx.m_message );
							}
					};

				return transform_indicator_t< Arg >{ limit, std::move( act ) };
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
		template< typename Source, typename Lambda >
		[[nodiscard]]
		static transform_indicator_t< Source >
		limit_then_transform(
			unsigned int limit,
			Lambda transformator )
			{
				ensure_signal< Source >();

				action_t act = [transformator]( const overlimit_context_t & ctx ) {
						const auto actual_transform = [&]() {
								auto r = transformator();
								impl::transform_reaction(
										ctx, r.mbox(), r.msg_type(), r.message() );
							};

						// Envelopes should be handled a special way.
						// Payload must be extrected and checked for presence.
						if( message_t::kind_t::enveloped_msg == message_kind(
								ctx.m_message ) )
							{
								const auto opt_payload = ::so_5::enveloped_msg::
										extract_payload_for_message_transformation(
												ctx.m_message );

								// Payload can be optional. Se we will perform
								// transformation only if payload is present.
								if( opt_payload )
									{
										actual_transform();
									}
							}
						else
							{
								actual_transform();
							}

					};

				return transform_indicator_t< Source >{ limit, std::move( act ) };
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
		template< typename Msg, typename... Args >
		[[nodiscard]]
		static transformed_message_t< Msg >
		make_transformed( mbox_t mbox, Args &&... args )
			{
				return transformed_message_t< Msg >::make(
						std::move( mbox ),
						std::forward<Args>( args )... );
			}
	};

} /* namespace message_limit */

} /* namespace so_5 */


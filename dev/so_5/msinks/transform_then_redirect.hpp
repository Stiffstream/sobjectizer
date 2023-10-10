/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff for transform_then_redirect message sink.
 *
 * \since v.5.8.1
 */

#pragma once

#include <so_5/message_sink.hpp>
#include <so_5/transformed_message.hpp>
#include <so_5/environment.hpp>

#include <so_5/details/lambda_traits.hpp>

#include <so_5/outliving.hpp>

#include <so_5/impl/msg_tracing_helpers.hpp>

#include <optional>
#include <type_traits>

namespace so_5
{

namespace msinks
{

namespace transform_then_redirect_impl
{

/*!
 * \brief Internal function to be used in %deliver_transformation_result.
 *
 * Performs message delivery tracing if msg_tracing is enabled.
 *
 * Should be used when transformation result is present and has to be
 * delivered to the destination mbox.
 *
 * \note
 * This function isn't a template and should minimize code size when
 * a template deliver_transformation_result is instantiated.
 */
inline void
trace_deliver_transformation_result_if_enabled(
	environment_t & env,
	const std::type_index & transformed_msg_type,
	const abstract_message_box_t & dest_mbox,
	message_delivery_mode_t delivery_mode,
	unsigned int redirection_deep )
	{
		so_5::impl::internal_env_iface_t internal_env{ env };
		if( internal_env.is_msg_tracing_enabled() )
			{
				namespace tracing = so_5::impl::msg_tracing_helpers::details;

				tracing::make_trace(
						// It's safe to use _nonchecked() because
						// is_msg_tracing_enabled() returns true.
						internal_env.msg_tracing_stuff_nonchecked(),
						tracing::composed_action_name{
								"msinks::transform_then_redirect",
								"deliver_transformed"
						},
						tracing::type_of_transformed_msg{ transformed_msg_type },
						tracing::mbox_as_msg_destination{ dest_mbox },
						delivery_mode,
						tracing::redirection_deep{ redirection_deep } );
			}
	}

/*!
 * \brief Internal function to be used in %deliver_transformation_result.
 *
 * Performs message delivery tracing if msg_tracing is enabled.
 *
 * Should be used when transformer returns an empty optional.
 *
 * \note
 * This function isn't a template and should minimize code size when
 * a template deliver_transformation_result is instantiated.
 */
inline void
trace_no_transformation_result_if_enabled(
	environment_t & env,
	message_delivery_mode_t delivery_mode,
	unsigned int redirection_deep )
	{
		so_5::impl::internal_env_iface_t internal_env{ env };
		if( internal_env.is_msg_tracing_enabled() )
			{
				namespace tracing = so_5::impl::msg_tracing_helpers::details;

				tracing::make_trace(
						// It's safe to use _nonchecked() because
						// is_msg_tracing_enabled() returns true.
						internal_env.msg_tracing_stuff_nonchecked(),
						tracing::composed_action_name{
								"msinks::transform_then_redirect",
								"no_transformation_result"
						},
						delivery_mode,
						tracing::redirection_deep{ redirection_deep } );
			}
	}

/*!
 * \brief Helper function for calling do_deliver_message for sending a
 * transformation result.
 *
 * This function is used when tranformation function returns just
 * so_5::transformed_message_t.
 *
 * \attention
 * \a redirection_deep is not incremented. It's expected that this
 * value is already processed by the caller.
 *
 * \since v.5.8.1
 */
template< typename Msg >
void
deliver_transformation_result(
	environment_t & env,
	message_delivery_mode_t delivery_mode,
	transformed_message_t< Msg > & r,
	unsigned int redirection_deep )
	{
		trace_deliver_transformation_result_if_enabled(
				env,
				r.msg_type(),
				*(r.mbox()),
				delivery_mode,
				redirection_deep );

		r.mbox()->do_deliver_message(
				delivery_mode,
				r.msg_type(),
				r.message(),
				redirection_deep );
	}

/*!
 * \brief Helper function for calling do_deliver_message for sending a
 * transformation result.
 *
 * This function is used when tranformation function returns
 * std::optional<so_5::transformed_message_t>.
 *
 * \note
 * Delivery attempt will be performed only if \a r contains a value.
 *
 * \attention
 * \a redirection_deep is not incremented. It's expected that this
 * value is already processed by the caller.
 *
 * \since v.5.8.1
 */
template< typename Msg >
void
deliver_transformation_result(
	environment_t & env,
	message_delivery_mode_t delivery_mode,
	std::optional< transformed_message_t< Msg > > & r,
	unsigned int redirection_deep )
	{
		if( r.has_value() )
			{
				deliver_transformation_result(
						env,
						delivery_mode,
						*r,
						redirection_deep );
			}
		else
			{
				trace_no_transformation_result_if_enabled(
						env,
						delivery_mode,
						redirection_deep );
			}
	}

/*!
 * \brief Helper function for causing a compilation error.
 *
 * This function will be selected by a compiler if transformation
 * function neither returns so_5::transformed_message_t nor
 * std::optional<so_5::transformed_message_t>.
 *
 * \since v.5.8.1
 */
template< typename Dummy >
void
deliver_transformation_result(
	environment_t & /*env*/,
	message_delivery_mode_t /*delivery_mode*/,
	const Dummy &,
	unsigned int /*redirection_deep*/ )
	{
		static_assert( so_5::details::always_false<Dummy>::value,
				"Transformer for transform_then_redirect_sink should "
				"return so_5::transformed_message_t<Msg> or "
				"std::optional<so_5::transformed_message_t<Msg>>" );
	}

/*!
 * \brief Base class for all implementation of transform_then_redirect sinks.
 *
 * Holds a reference to SObjectizer Environment.
 * This reference is necessary for implementation of inherited environment() method.
 *
 * Holds the tranformer.
 *
 * Implements all inherited methods with respect to enveloped messages and so on.
 *
 * Introduces a new virtual method call_transformer_then_go_further() that has to
 * be implemented in derived classes.
 *
 * \note
 * This implementation always returns so_5::prio::p0 as sink priority.
 *
 * \tparam Transformer type of message tranformer.
 *
 * \since v.5.8.1
 */
template< typename Transformer >
class basic_transform_then_redirect_sink_t
	:	public abstract_message_sink_t
	{
	protected:
		//! SObjectizer Environment in that we're working.
		outliving_reference_t< so_5::environment_t > m_env;

		//! Functor to be used for transformation.
		Transformer m_transformer;

	public:
		/*!
		 * \brief Initializing constructor.
		 *
		 * \tparam Transformer_T type of the transformed. It's expected to be
		 * the same type as \a Transformer, but we're using template constructor
		 * here to have a possibility to use perfect forwarding.
		 */
		template< typename Transformer_T >
		basic_transform_then_redirect_sink_t(
			//! SObjectizer Environment to work in.
			outliving_reference_t< so_5::environment_t > env,
			//! Functor to be used for transformation.
			Transformer_T && transformer )
			:	m_env{ env }
			,	m_transformer{ std::forward<Transformer_T>(transformer) }
			{}

		[[nodiscard]]
		environment_t &
		environment() const noexcept final
			{
				return m_env.get();
			}

		[[nodiscard]]
		priority_t
		sink_priority() const noexcept final
			{
				return prio::p0;
			}

		void
		push_event(
			mbox_id_t mbox_id,
			message_delivery_mode_t delivery_mode,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int redirection_deep,
			const message_limit::impl::action_msg_tracer_t * /*tracer*/ ) final
			{
				if( redirection_deep >= max_redirection_deep )
					{
						// NOTE: this fragment can throw but it isn't a problem
						// because push_event() is called during message
						// delivery process and exceptions are expected in that
						// process.
						SO_5_LOG_ERROR(
								this->environment().error_logger(),
								logger )
							logger << "maximum message redirection deep exceeded on "
									"tranform_then_redirect_sink_t::push_event; "
									"message will be ignored;"
								<< " msg_type: " << msg_type.name()
								<< ", mbox_id: " << mbox_id;
					}
				else
					{
						this->handle_envelope_then_go_further(
								delivery_mode,
								message,
								// redirection_deep has to be increased.
								redirection_deep + 1u );
					}
			}

	private:
		void
		handle_envelope_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & message,
			unsigned int redirection_deep )
			{
				// Envelopes should be handled a special way.
				// Payload must be extrected and checked for presence.
				if( message_t::kind_t::enveloped_msg == message_kind( message ) )
					{
						const auto opt_payload = ::so_5::enveloped_msg::
								extract_payload_for_message_transformation( message );

						// Payload can be optional. Se we will perform
						// transformation only if payload is present.
						if( opt_payload )
							{
								call_transformer_then_go_further(
										delivery_mode,
										opt_payload->message(),
										redirection_deep );
							}
					}
				else
					{
						call_transformer_then_go_further(
								delivery_mode,
								message,
								redirection_deep );
					}
			}

		/*!
		 * \brief Method that does actual transformation and redirection
		 * of the message.
		 *
		 * It's called when envelopes already processed and \a message points
		 * to the actual payload (if it's not a signal).
		 *
		 * \note
		 * The value of \a redirection_deep is already checked and incremented.
		 */
		virtual void
		call_transformer_then_go_further(
			//! Delivery mode to be used. This is the value form push_event() method.
			message_delivery_mode_t delivery_mode,
			//! Reference to payload to be transformed.
			//! It has to be nullptr for a signal.
			const message_ref_t & message,
			//! Value of redirection deep to be used for delivery of transformation
			//! result.
			unsigned int redirection_deep ) = 0;
	};

/*!
 * \brief Implementation of tranformation sink for a case when
 * a message has to be transformed.
 *
 * \tparam Expected_Msg type of source message. It can be M,
 * so_5::immutable_msg<M> or so_5::mutable_msg<M>.
 *
 * \tparam Transformer type of message tranformer.
 *
 * \since v.5.8.1
 */
template<
	typename Expected_Msg,
	typename Transformer >
class msg_transform_then_redirect_sink_t final
	:	public basic_transform_then_redirect_sink_t< Transformer >
	{
		//! Type of a payload of the message.
		using payload_t =
				typename message_payload_type< Expected_Msg >::payload_type;

		//! Type of a reference to the message payload.
		/*!
		 * In the case of an immutable message it should be const reference.
		 * In the case of a mutable message it should be non-nonst reference.
		 */
		using payload_reference_t = std::conditional_t<
				is_mutable_message< Expected_Msg >::value,
				payload_t &,
				const payload_t &
			>;

		//! Short alias for the base type.
		using base_type_t = basic_transform_then_redirect_sink_t< Transformer >;

	public:
		// Just reuse the constructor(s).
		using base_type_t::base_type_t;

	private:
		/*!
		 * \brief Actual implementation of transformation and delivery of the result.
		 *
		 * Extracts the payload from \a message. Calls the transformer and delivers
		 * the result (if result is present).
		 */
		void
		call_transformer_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & message,
			unsigned int redirection_deep ) override
			{
				payload_reference_t payload = message_payload_type< Expected_Msg >::
						payload_reference( *message.get() );
				auto r = this->m_transformer( payload );

				deliver_transformation_result(
						this->m_env.get(),
						delivery_mode,
						r,
						redirection_deep );
			}
	};

/*!
 * \brief Implementation of tranformation sink for a case when
 * a message has to be transformed.
 *
 * \tparam Signal type of source signal.
 *
 * \tparam Transformer type of message tranformer.
 *
 * \since v.5.8.1
 */
template< typename Signal, typename Transformer >
class signal_transform_then_redirect_sink_t final
	:	public basic_transform_then_redirect_sink_t< Transformer >
	{
		//! Short alias for the base type.
		using base_type_t = basic_transform_then_redirect_sink_t< Transformer >;

	public:
		// Just reuse the constructor(s).
		using base_type_t::base_type_t;

	private:
		/*!
		 * \brief Actual implementation of transformation and delivery of the result.
		 *
		 * Extracts the payload from \a message. Calls the transformer and delivers
		 * the result (if result is present).
		 */
		void
		call_transformer_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & /*message*/,
			unsigned int redirection_deep ) override
			{
				auto r = this->m_transformer();

				deliver_transformation_result(
						this->m_env.get(),
						delivery_mode,
						r,
						redirection_deep );
			}
	};

} /* namespace transform_then_redirect_impl */

//FIXME: document this!
template< typename Transformer_Lambda >
[[nodiscard]]
msink_t
transform_then_redirect(
	so_5::environment_t & env,
	Transformer_Lambda && transformer )
	{
		using namespace transform_then_redirect_impl;

		using transformer_t = std::decay_t< Transformer_Lambda >;
		using lambda_traits_t =
				so_5::details::lambda_traits::traits< transformer_t >;
		using transformer_arg_t =
				std::remove_cv_t< typename lambda_traits_t::argument_type >;
		using sink_t = msg_transform_then_redirect_sink_t<
				transformer_arg_t, // As expected msg type.
				transformer_t >;
//FIXME: should here be checks against so_5::mutable_msg<M> and so_5::immutable_msg<M>?
		using sink_owner_t = simple_sink_owner_t< sink_t >;

		return {
				std::make_unique< sink_owner_t >(
						outliving_mutable( env ),
						std::forward<Transformer_Lambda>(transformer) )
			};
	}

//FIXME: document this!
template< typename Expected_Msg, typename Transformer_Lambda >
[[nodiscard]]
std::enable_if_t< !is_signal<Expected_Msg>::value, msink_t >
transform_then_redirect(
	so_5::environment_t & env,
	Transformer_Lambda && transformer )
	{
		// Just for a case (it's a bit paranoid).
		ensure_not_signal< Expected_Msg >();

		using namespace transform_then_redirect_impl;

		using transformer_t = std::decay_t< Transformer_Lambda >;
		using sink_t = msg_transform_then_redirect_sink_t<
				Expected_Msg, // As expected msg type.
				transformer_t >;
		using sink_owner_t = simple_sink_owner_t< sink_t >;

		return {
				std::make_unique< sink_owner_t >(
						outliving_mutable( env ),
						std::forward<Transformer_Lambda>(transformer) )
			};
	}

//FIXME: document this!
template< typename Signal, typename Transformer_Lambda >
[[nodiscard]]
std::enable_if_t< is_signal<Signal>::value, msink_t >
transform_then_redirect(
	so_5::environment_t & env,
	Transformer_Lambda && transformer )
	{
		// Just for a case (it's a bit paranoid).
		ensure_signal< Signal >();

		using namespace transform_then_redirect_impl;

		using transformer_t = std::decay_t< Transformer_Lambda >;
		using sink_t = signal_transform_then_redirect_sink_t< Signal, transformer_t >;
		using sink_owner_t = simple_sink_owner_t< sink_t >;

		return {
				std::make_unique< sink_owner_t >(
						outliving_mutable( env ),
						std::forward<Transformer_Lambda>(transformer) )
			};
	}

} /* namespace msinks */

} /* namespace so_5 */


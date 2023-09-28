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

#include <optional>
#include <type_traits>

namespace so_5
{

namespace msinks
{

namespace transform_then_redirect_impl
{

//FIXME: document this!
template< typename Msg >
void
deliver_transformation_result(
	message_delivery_mode_t delivery_mode,
	transformed_message_t< Msg > & r,
	unsigned int redirection_deep )
	{
		r.mbox()->do_deliver_message(
				delivery_mode,
				r.msg_type(),
				r.message(),
				redirection_deep );
	}

//FIXME: document this!
template< typename Msg >
void
deliver_transformation_result(
	message_delivery_mode_t delivery_mode,
	std::optional< transformed_message_t< Msg > > & r,
	unsigned int redirection_deep )
	{
		if( r.has_value() )
			{
				deliver_transformation_result(
						delivery_mode,
						*r,
						redirection_deep );
			}
	}

//FIXME: document this!
template< typename Dummy >
void
deliver_transformation_result(
	message_delivery_mode_t /*delivery_mode*/,
	const Dummy &,
	unsigned int /*redirection_deep*/ )
	{
		static_assert( so_5::details::always_false<Dummy>::value,
				"Transformer for transform_then_redirect_sink should "
				"return so_5::transformed_message_t<Msg> or "
				"std::optional<so_5::transformed_message_t<Msg>>" );
	}

//FIXME: document this!
template< typename Transformer >
class basic_transform_then_redirect_sink_t
	:	public abstract_message_sink_t
	{
	protected:
		outliving_reference_t< so_5::environment_t > m_env;

		Transformer m_transformer;

	public:
		template< typename Transformer_T >
		basic_transform_then_redirect_sink_t(
			outliving_reference_t< so_5::environment_t > env,
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

		//FIXME: should this value be specified in the constructor?
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

		virtual void
		call_transformer_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & message,
			unsigned int redirection_deep ) = 0;
	};

//FIXME: document this!
template<
	typename Expected_Msg,
	typename Transformer >
class msg_transform_then_redirect_sink_t final
	:	public basic_transform_then_redirect_sink_t< Transformer >
	{
		using payload_t =
				typename message_payload_type< Expected_Msg >::payload_type;

		using payload_reference_t = std::conditional_t<
				is_mutable_message< Expected_Msg >::value,
				payload_t &,
				const payload_t &
			>;

		using base_type_t = basic_transform_then_redirect_sink_t< Transformer >;

	public:
		using base_type_t::base_type_t;

	private:
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
						delivery_mode,
						r,
						redirection_deep );
			}
	};

//FIXME: document this!
template< typename Signal, typename Transformer >
class signal_transform_then_redirect_sink_t final
	:	public basic_transform_then_redirect_sink_t< Transformer >
	{
		using base_type_t = basic_transform_then_redirect_sink_t< Transformer >;

	public:
		using base_type_t::base_type_t;

	private:
		void
		call_transformer_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & /*message*/,
			unsigned int redirection_deep ) override
			{
				auto r = this->m_transformer();

				deliver_transformation_result(
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


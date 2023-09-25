namespace transform_then_redirect_sink_impl
{

template< typename Msg >
void
deliver_transformation_result(
	transformed_message_t< Msg > & r,
	unsigned int redirection_deep )
	{
		r.mbox()->do_deliver_message(
				delivery_mode,
				r.msg_type(),
				r.message(),
				redirection_deep );
	}

template< typename Msg >
void
deliver_transformation_result(
	std::optional< transformed_message_t< Msg > > r,
	unsigned int redirection_deep )
	{
		if( r.has_value() )
			deliver_transformation_result( *r );
	}

template< typename Dummy >
void
deliver_transformation_result(
	const Dummy &,
	unsigned int redirection_deep )
	{
		static_assert( so_5::details::always_false<Dummy>::value,
				"Transformer for transform_then_redirect_sink should "
				"return so_5::transformed_message_t<Msg> or "
				"std::optional<so_5::transformed_message_t<Msg>>" );
	}

} /* namespace transform_then_redirect_sink_impl */

template< typename Transformer >
class tranform_then_redirect_sink_t final : public abstract_message_sink_t
	{
		payload_t = typename so_5::details::lambda_traits::
				argument_type_if_lambda< Lambda >::type;

		outliving_reference_t< so_5::environment_t > m_env;

		Transformer m_transformer;

	public:
		template< typename Transformer_T >
		tranform_then_redirect_sink_t(
			outliving_reference_t< so_5::environment_t > env,
			Transformer_T && transformer )
			:	m_env{ env }
			,	m_transformer{ std::forward<Transformer_T>(transformer) }
			{}

		[[nodiscard]]
		environment_t &
		environment() const noexcept override
			{
				return m_env.get();
			}

		//FIXME: should this value be specified in the constructor?
		[[nodiscard]]
		priority_t
		sink_priority() const noexcept override
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
			const message_limit::impl::action_msg_tracer_t * /*tracer*/ )
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
								mbox_id,
								delivery_mode,
								msg_type,
								message,
								// redirection_deep has to be increased.
								redirection_deep + 1u );
					}
			}

	private:
		void
		handle_envelope_then_go_further(
			mbox_id_t /*mbox_id*/,
			message_delivery_mode_t delivery_mode,
			const std::type_index & /*msg_type*/,
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

		void
		call_transformer_then_go_further(
			message_delivery_mode_t delivery_mode,
			const message_ref_t & message,
			unsigned int redirection_deep )
			{
				const auto & payload =
						message_payload_type< payload_t >::payload_reference(
								*(message.get()) );
				auto r = m_transformer( payload );

				using transform_then_redirect_sink_impl::deliver_transformation_result;
				deliver_transformation_result( r, redirection_deep + 1u );
			}
	};


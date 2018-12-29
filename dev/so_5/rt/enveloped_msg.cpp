/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.23
 *
 * \brief Stuff related to enveloped messages.
 */

#include <so_5/rt/h/enveloped_msg.hpp>

#include <so_5/rt/impl/h/enveloped_msg_details.hpp>

namespace so_5 {

namespace enveloped_msg {

namespace {

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// payload_access_handler_invoker_t
//
/*!
 * \brief An implementation of handler_invoker interface for
 * extraction of payload because of various reasons.
 *
 * \since
 * v.5.5.23
 */
class payload_access_handler_invoker_t final : public handler_invoker_t
	{
		const access_context_t m_context;

		optional< ::so_5::enveloped_msg::payload_info_t > m_payload;

	public:
		payload_access_handler_invoker_t(
			access_context_t context )
			:	m_context{ context }
			{}

		~payload_access_handler_invoker_t() = default;

		void
		invoke(
			const payload_info_t & payload ) SO_5_NOEXCEPT override
			{
				using namespace so_5::enveloped_msg::impl;

				switch( detect_invocation_type_for_message( payload.message() ) )
					{
					case invocation_type_t::event :
						m_payload = payload;
					break;

					case invocation_type_t::service_request :
						m_payload = payload;
					break;

					case invocation_type_t::enveloped_msg :
						auto & envelope = message_to_envelope( payload.message() );
						envelope.access_hook( m_context, *this );
					break;
					}
			}

		SO_5_NODISCARD
		optional< so_5::enveloped_msg::payload_info_t >
		try_get_payload(
			const message_ref_t & envelope_to_process ) SO_5_NOEXCEPT
			{
				using namespace so_5::enveloped_msg::impl;

				auto & actual_envelope = message_to_envelope( envelope_to_process );
				actual_envelope.access_hook( m_context, *this );

				return m_payload;
			}
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace anonymous */

SO_5_NODISCARD
SO_5_FUNC
optional< payload_info_t >
extract_payload_for_message_transformation(
	const message_ref_t & envelope_to_process )
	{
		return payload_access_handler_invoker_t{
						access_context_t::transformation
				}.try_get_payload( envelope_to_process );
	}
	
SO_5_NODISCARD
SO_5_FUNC
optional< message_ref_t >
message_to_be_inspected(
	const message_ref_t & msg_or_envelope )
	{
		message_ref_t result = msg_or_envelope;
		switch( message_kind( msg_or_envelope ) )
			{
			case message_t::kind_t::signal: // Already has a value.
			break;

			case message_t::kind_t::classical_message: // Already has a value.
			break;

			case message_t::kind_t::user_type_message: // Already has a value.
			break;

			case message_t::kind_t::service_request: // Already has a value.
			break;

			case message_t::kind_t::enveloped_msg:
				{
					auto opt_payload = payload_access_handler_invoker_t{
									access_context_t::inspection
							}.try_get_payload( msg_or_envelope );

					if( opt_payload )
						result = opt_payload->message();
				}
			break;
			}

		return { result };
	}

} /* namespace enveloped_msg */

} /* namespace so_5 */


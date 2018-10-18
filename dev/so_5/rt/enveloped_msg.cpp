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
// on_transform_handler_invoker_t
//
/*!
 * \brief An implementation of handler_invoker interface for
 * the case of message transformation.
 *
 * \since
 * v.5.5.23
 */
class on_transform_handler_invoker_t final : public handler_invoker_t
	{
		optional< ::so_5::enveloped_msg::payload_info_t > m_payload;

	public:
		on_transform_handler_invoker_t() = default;
		~on_transform_handler_invoker_t() = default;

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
						envelope.transformation_hook( *this );
					break;
					}
			}

		SO_5_NODISCARD
		optional< ::so_5::enveloped_msg::payload_info_t >
		payload() const SO_5_NOEXCEPT
			{
				return m_payload;
			}
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace anonymous */

SO_5_FUNC
SO_5_NODISCARD
optional< payload_info_t >
extract_payload_for_message_transformation(
	const message_ref_t & envelope_to_process )
	{
		using namespace so_5::enveloped_msg::impl;

		auto & actual_envelope = message_to_envelope( envelope_to_process );
		on_transform_handler_invoker_t invoker;

		actual_envelope.transformation_hook( invoker );

		return invoker.payload();
	}
	
} /* namespace enveloped_msg */

} /* namespace so_5 */


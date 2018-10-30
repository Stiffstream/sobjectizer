/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.23
 *
 * \brief Some implementation details for enveloped messages.
 */
#pragma once

#include <so_5/rt/h/enveloped_msg.hpp>
#include <so_5/rt/h/agent.hpp>

#include <so_5/rt/impl/h/subscription_storage_iface.hpp>

#include <so_5/details/h/abort_on_fatal_error.hpp>

#include <so_5/h/ret_code.hpp>

namespace so_5 {

namespace enveloped_msg {

namespace impl {

//
// message_to_envelope
//
/*!
 * \brief A helper function for casting message instance to
 * envelope instance.
 *
 * \note
 * This function throws if message is not an instance of envelope_t.
 *
 * \note
 * This function throws if \a src_msg is nullptr.
 *
 * \since
 * v.5.5.23
 */
SO_5_NODISCARD
inline envelope_t &
message_to_envelope(
	//! Message for casting.
	const message_ref_t & src_msg )
	{
		// Pointer to message object can't be null. We can't continue our work
		// if it is null.
		auto * raw_msg_ptr = src_msg.get();
		if( !raw_msg_ptr )
			SO_5_THROW_EXCEPTION(
					rc_attempt_to_cast_to_envelope_on_nullptr,
					std::string( "Unexpected error: pointer to enveloped_msg is null." ) );


		// If message is not instance of envelope_t an exception will be
		// thrown here by C++ run-time.
		return dynamic_cast<enveloped_msg::envelope_t &>(*raw_msg_ptr);
	}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

//
// agent_demand_handler_invoker_t
//
/*!
 * \brief An implementation of handler_invoker interface.
 *
 * \since
 * v.5.5.23
 */
class agent_demand_handler_invoker_t : public handler_invoker_t
	{
		const current_thread_id_t m_work_thread_id;
		execution_demand_t & m_demand;
		const so_5::impl::event_handler_data_t & m_handler_data;

	public:
		//! Initializing constructor.
		agent_demand_handler_invoker_t(
			current_thread_id_t work_thread_id,
			execution_demand_t & demand,
			const so_5::impl::event_handler_data_t & handler_data )
			:	m_work_thread_id( work_thread_id )
			,	m_demand( demand )
			,	m_handler_data( handler_data )
			{}
		~agent_demand_handler_invoker_t() = default;

		void
		invoke( const payload_info_t & payload ) SO_5_NOEXCEPT override
			{
				const auto invocation_type = detect_invocation_type_for_message(
						payload.message() );

				execution_demand_t fresh_demand{
						m_demand.m_receiver,
						m_demand.m_limit,
						m_demand.m_mbox_id,
						m_demand.m_msg_type,
						payload.message(),
						// May be it is not necessary at all but it
						// is better to have properly constructed demand.
						demand_handler_for_invocation_type( invocation_type )
				};

				switch( invocation_type )
					{
					case invocation_type_t::event :
						agent_t::process_message(
								m_work_thread_id,
								fresh_demand,
								m_handler_data.m_method );
					break;

					case invocation_type_t::service_request :
						agent_t::process_service_request(
								m_work_thread_id,
								fresh_demand,
								std::make_pair( true, &m_handler_data ) );
					break;

					case invocation_type_t::enveloped_msg :
						agent_t::process_enveloped_msg(
								m_work_thread_id,
								fresh_demand,
								&m_handler_data );
					break;
					}
			}

	private:
		/*!
		 * Returns appropriate pointer to demand handler in dependency
		 * on invocation type for that demand.
		 */
		static demand_handler_pfn_t
		demand_handler_for_invocation_type(
			invocation_type_t type ) SO_5_NOEXCEPT
			{
				demand_handler_pfn_t result = nullptr;
				switch( type )
					{
					case invocation_type_t::event :
						result = &agent_t::demand_handler_on_message;
					break;

					case invocation_type_t::service_request :
						result = &agent_t::demand_handler_on_service_request;
					break;

					case invocation_type_t::enveloped_msg :
						result = &agent_t::demand_handler_on_enveloped_msg;
					break;
					}
				return result;
			}
	};

//
// mchain_demand_handler_invoker_t
//
/*!
 * \brief An implementation of handler_invoker interface for
 * the case when a mesage was sent to mchain.
 *
 * \since
 * v.5.5.23
 */
class mchain_demand_handler_invoker_t : public handler_invoker_t
	{
		const ::so_5::details::msg_type_and_handler_pair_t & m_handler;

		bool m_was_handled;

	public:
		//! Initializing constructor.
		mchain_demand_handler_invoker_t(
			const ::so_5::details::msg_type_and_handler_pair_t & handler )
			:	m_handler( handler )
			,	m_was_handled( false )
			{}
		~mchain_demand_handler_invoker_t() = default;

		bool
		was_handled() const SO_5_NOEXCEPT { return m_was_handled; }

		void
		invoke( const payload_info_t & payload ) SO_5_NOEXCEPT override
			{
				const auto invocation_type = detect_invocation_type_for_message(
						payload.message() );

				switch( invocation_type )
					{
					case invocation_type_t::event :
						m_was_handled = true;
						m_handler.m_handler( invocation_type, payload.message() );
					break;

					case invocation_type_t::service_request :
						m_was_handled = true;
						// Invocation should be done in a special wrapper.
						msg_service_request_base_t::dispatch_wrapper(
								payload.message(),
								[this, &payload] {
									m_handler.m_handler(
											invocation_type_t::service_request,
											payload.message() );
								} );
					break;

					case invocation_type_t::enveloped_msg :
						{
							// Do recurvise call.
							// Value for was_handled will be detected in the
							// nested call.
							auto & nested_envelope = message_to_envelope(
									payload.message() );
							nested_envelope.access_hook(
									access_context_t::handler_found,
									*this );
						}
					break;
					}
			}
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace impl */

} /* namespace enveloped_msg */

} /* namespace so_5 */


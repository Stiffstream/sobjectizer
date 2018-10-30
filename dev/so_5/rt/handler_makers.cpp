/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Implementation of some parts of handler makers which can't be inline.
 *
 * \since
 * v.5.5.23
 */

#include <so_5/rt/h/handler_makers.hpp>

#include <so_5/rt/impl/h/enveloped_msg_details.hpp>

namespace so_5 {

namespace details {

namespace {

bool
process_envelope_when_handler_found(
	const msg_type_and_handler_pair_t & handler,
	message_ref_t & message )
	{
		using namespace so_5::enveloped_msg::impl;

		bool result = false;

		// We don't expect exceptions here and can't restore after them.
		so_5::details::invoke_noexcept_code( [&] {
			auto & envelope = message_to_envelope( message );

			mchain_demand_handler_invoker_t invoker{ handler };
			envelope.access_hook(
					so_5::enveloped_msg::access_context_t::handler_found,
					invoker );

			result = invoker.was_handled();
		} );

		return result;
	}

} /* namespace anonymous */

SO_5_FUNC
bool
handlers_bunch_basics_t::find_and_use_handler(
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
				switch( invocation )
					{
					case invocation_type_t::event :
						// This is an async message.
						// Simple call is enough.
						ret_value = true;
						it->m_handler( invocation, message );
					break;

					case invocation_type_t::service_request :
						ret_value = true;
						// Invocation should be done in a special wrapper.
						msg_service_request_base_t::dispatch_wrapper(
								message,
								[&it, &message, invocation] {
									it->m_handler( invocation, message );
								} );
					break;

					case invocation_type_t::enveloped_msg :
						// Invocation must be done a special way.
						ret_value = process_envelope_when_handler_found( *it, message );
					break;
					}
			}

		return ret_value;
	}

} /* namespace details */

} /* namespace so_5 */


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

#include <so_5/handler_makers.hpp>

#include <so_5/impl/enveloped_msg_details.hpp>

namespace so_5 {

namespace details {

namespace {

//FIXME: document this!
bool
process_envelope_when_handler_found(
	const msg_type_and_handler_pair_t & handler,
	message_ref_t & message )
	{
		using namespace so_5::enveloped_msg::impl;

		bool result = false;

//FIXME: is this part of code has to be noexcept?
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

//FIXME: document this!
so_5::cpp_coro::task_t< bool >
async_process_envelope_when_handler_found(
	const msg_type_and_handler_pair_t & handler,
	message_ref_t message )
	{
		using namespace so_5::enveloped_msg::impl;

		auto & envelope = message_to_envelope( message );

		mchain_demand_handler_invoker_t invoker{ handler };
		co_await envelope.access_hook_async(
				so_5::enveloped_msg::async_access_context_t::handler_found,
				invoker );

		co_return invoker.was_handled();
	}

} /* namespace anonymous */

SO_5_FUNC
bool
handlers_bunch_basics_t::find_and_use_handler(
	const msg_type_and_handler_pair_t * left,
	const msg_type_and_handler_pair_t * right,
	const std::type_index & msg_type,
	message_ref_t & message )
	{
		bool ret_value = false;

		msg_type_and_handler_pair_t key{ msg_type };
		auto it = std::lower_bound( left, right, key );
		if( it != right && it->m_msg_type == key.m_msg_type )
			{
				// Handler is found and must be called.
				switch( message_kind( message ) )
					{
					case message_t::kind_t::signal : [[fallthrough]];
					case message_t::kind_t::classical_message : [[fallthrough]];
					case message_t::kind_t::user_type_message :
						// This is an async message.
						// Simple call is enough.
						ret_value = true;
						so_5::low_level_api::invoke_sync_event_handler(
								it->m_handler,
								message );
					break;

					case message_t::kind_t::enveloped_msg :
						// Invocation must be done a special way.
						ret_value = process_envelope_when_handler_found( *it, message );
					break;
					}
			}

		return ret_value;
	}

so_5::cpp_coro::task_t< bool >
handlers_bunch_basics_t::find_and_use_handler_async(
	const msg_type_and_handler_pair_t * left,
	const msg_type_and_handler_pair_t * right,
	const std::type_index & msg_type,
	message_ref_t message )
	{
		bool ret_value = false;

		msg_type_and_handler_pair_t key{ msg_type };
		auto it = std::lower_bound( left, right, key );
		if( it != right && it->m_msg_type == key.m_msg_type )
			{
				// Handler is found and must be called.
				switch( message_kind( message ) )
					{
					case message_t::kind_t::signal : [[fallthrough]];
					case message_t::kind_t::classical_message : [[fallthrough]];
					case message_t::kind_t::user_type_message :
						ret_value = true;

						using so_5::low_level_api::any_handler_as_async_invoker_t;
						co_await std::visit(
								any_handler_as_async_invoker_t{ message },
								it->m_handler );
					break;

					case message_t::kind_t::enveloped_msg :
						// Invocation must be done a special way.
						ret_value = co_await async_process_envelope_when_handler_found(
								*it, std::move(message) );
					break;
					}
			}

		co_return ret_value;
	}

} /* namespace details */

} /* namespace so_5 */


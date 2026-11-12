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
[[nodiscard]]
find_then_call_result_t
process_envelope_when_handler_found(
	const msg_type_and_handler_pair_t & handler,
	message_ref_t & message )
	{
		using namespace so_5::enveloped_msg::impl;

		// NOTE: an exception can be raised here. It shouldn't be a problem.
		auto & envelope = message_to_envelope( message );

		// NOTE: the access_hook is noexcept (at least for v.5.8.7) and
		// we don't care about exceptions here.
		mchain_demand_handler_invoker_t invoker{ handler };
		envelope.access_hook(
				so_5::enveloped_msg::access_context_t::handler_found,
				invoker );

		return invoker.was_handled()
				? find_then_call_result_t::found_and_called
				: find_then_call_result_t::not_found;
	}

} /* namespace anonymous */

SO_5_FUNC
find_then_call_result_t
handlers_bunch_basics_t::find_and_use_handler(
	const msg_type_and_handler_pair_t * left,
	const msg_type_and_handler_pair_t * right,
	const std::type_index & msg_type,
	message_ref_t & message )
	{
		auto ret_value = find_then_call_result_t::not_found;

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
						ret_value = find_then_call_result_t::found_and_called;
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

so_5::cpp_coro::task_t< find_then_call_result_t >
handlers_bunch_basics_t::find_and_use_handler_async(
	const msg_type_and_handler_pair_t * left,
	const msg_type_and_handler_pair_t * right,
	std::type_index msg_type,
	message_ref_t message )
	{
		find_then_call_result_t ret_value = find_then_call_result_t::not_found;

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
						ret_value = find_then_call_result_t::found_and_called;

						using so_5::low_level_api::any_handler_as_async_invoker_t;
						co_await std::visit(
								any_handler_as_async_invoker_t{ message },
								it->m_handler );
					break;

					case message_t::kind_t::enveloped_msg :
						{
							// Invocation must be done a special way.
							using namespace so_5::enveloped_msg::impl;

							// Note: all actions a performed here to avoid
							// creation of another nested coroutine.
							auto & envelope = message_to_envelope( message );

							mchain_demand_handler_invoker_t invoker{ *it };
							co_await envelope.async_access_hook(
									so_5::enveloped_msg::async_access_context_t
											::handler_found,
									invoker );

							ret_value = invoker.was_handled()
									? find_then_call_result_t::found_and_called
									: find_then_call_result_t::not_found;
						}
					break;
					}
			}

		co_return ret_value;
	}

} /* namespace details */

} /* namespace so_5 */


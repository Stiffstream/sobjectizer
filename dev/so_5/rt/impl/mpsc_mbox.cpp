/*
	SObjectizer 5.
*/

#include <algorithm>
#include <sstream>

#include <so_5/rt/impl/h/mpsc_mbox.hpp>
#include <so_5/rt/impl/h/message_limit_internals.hpp>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/agent.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// limitless_mpsc_mbox_t
//

limitless_mpsc_mbox_t::limitless_mpsc_mbox_t(
	mbox_id_t id,
	agent_t * single_consumer )
	:	m_id( id )
	,	m_single_consumer( single_consumer )
{
}

limitless_mpsc_mbox_t::~limitless_mpsc_mbox_t()
{
}

void
limitless_mpsc_mbox_t::subscribe_event_handler(
	const std::type_index & /*type_wrapper*/,
	const so_5::rt::message_limit::control_block_t * /*limit*/,
	agent_t * subscriber )
{
	if( subscriber != m_single_consumer )
		SO_5_THROW_EXCEPTION(
				rc_illegal_subscriber_for_mpsc_mbox,
				"the only one consumer can create subscription to mpsc_mbox" );
}

void
limitless_mpsc_mbox_t::unsubscribe_event_handlers(
	const std::type_index & /*type_wrapper*/,
	agent_t * /*subscriber*/ )
{
	// Nothing to do.
}

std::string
limitless_mpsc_mbox_t::query_name() const
{
	std::ostringstream s;
	s << "<mbox:type=MPSC:id="
			<< m_id << ":consumer=" << m_single_consumer
			<< ">";

	return s.str();
}

void
limitless_mpsc_mbox_t::do_deliver_message(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int ) const
{
	agent_t::call_push_event(
			*m_single_consumer,
			so_5::rt::message_limit::control_block_t::none(),
			m_id,
			msg_type,
			message );
}

void
limitless_mpsc_mbox_t::do_deliver_service_request(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int ) const
{
	msg_service_request_base_t::dispatch_wrapper( message,
		[&] {
			agent_t::call_push_service_request(
					*m_single_consumer,
					so_5::rt::message_limit::control_block_t::none(),
					m_id,
					msg_type,
					message );
		} );
}

void
limitless_mpsc_mbox_t::set_delivery_filter(
	const std::type_index & /*msg_type*/,
	const delivery_filter_t & /*filter*/,
	agent_t & /*subscriber*/ )
{
	SO_5_THROW_EXCEPTION(
			rc_delivery_filter_cannot_be_used_on_mpsc_mbox,
			"set_delivery_filter is called for MPSC-mbox" );
}

void
limitless_mpsc_mbox_t::drop_delivery_filter(
	const std::type_index & /*msg_type*/,
	agent_t & /*subscriber*/ ) SO_5_NOEXCEPT
{
}

//
// limitful_mpsc_mbox_t
//

limitful_mpsc_mbox_t::limitful_mpsc_mbox_t(
	mbox_id_t id,
	agent_t * single_consumer,
	const so_5::rt::message_limit::impl::info_storage_t & limits_storage )
	:	limitless_mpsc_mbox_t( id, single_consumer )
	,	m_limits( limits_storage )
{
}

limitful_mpsc_mbox_t::~limitful_mpsc_mbox_t()
{
}

void
limitful_mpsc_mbox_t::do_deliver_message(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	using namespace so_5::rt::message_limit::impl;

	auto limit = m_limits.find( msg_type );

	try_to_deliver_to_agent< invocation_type_t::event >(
			*m_single_consumer,
			limit,
			msg_type,
			message,
			overlimit_reaction_deep,
			[&] {
				agent_t::call_push_event(
						*m_single_consumer,
						limit,
						m_id,
						msg_type,
						message );
			} );
}

void
limitful_mpsc_mbox_t::do_deliver_service_request(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	using namespace so_5::rt::message_limit::impl;

	msg_service_request_base_t::dispatch_wrapper( message,
		[&] {
			auto limit = m_limits.find( msg_type );

			try_to_deliver_to_agent< invocation_type_t::service_request >(
					*m_single_consumer,
					limit,
					msg_type,
					message,
					overlimit_reaction_deep,
					[&] {
						agent_t::call_push_service_request(
								*m_single_consumer,
								limit,
								m_id,
								msg_type,
								message );
					} );
		} );
}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


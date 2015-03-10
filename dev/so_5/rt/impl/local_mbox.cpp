/*
	SObjectizer 5.
*/

#include <algorithm>
#include <sstream>
#include <mutex>

#include <so_5/rt/impl/h/local_mbox.hpp>
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
// local_mbox_t
//

local_mbox_t::local_mbox_t(
	mbox_id_t id )
	:	m_id( id )
{
}

local_mbox_t::~local_mbox_t()
{
}

void
local_mbox_t::subscribe_event_handler(
	const std::type_index & type_wrapper,
	const so_5::rt::message_limit::control_block_t * limit,
	agent_t * subscriber )
{
	std::unique_lock< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( type_wrapper );
	if( it == m_subscribers.end() )
	{
		// There isn't such message type yet.
		subscriber_container_t container;
		container.emplace_back( subscriber, limit );

		m_subscribers.emplace( type_wrapper, std::move( container ) );
	}
	else
	{
		auto & agents = it->second;

		subscriber_info_t info{ subscriber, limit };

		auto pos = std::lower_bound( agents.begin(), agents.end(), info );
		if( pos != agents.end() )
		{
			// This is subscriber or appopriate place for it.
			if( pos->m_agent != subscriber )
				agents.insert( pos, info );
		}
		else
			agents.push_back( info );
	}
}

void
local_mbox_t::unsubscribe_event_handlers(
	const std::type_index & type_wrapper,
	agent_t * subscriber )
{
	std::unique_lock< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( type_wrapper );
	if( it != m_subscribers.end() )
	{
		auto & agents = it->second;

		auto pos = std::lower_bound( agents.begin(), agents.end(),
				subscriber_info_t{ subscriber, nullptr } );
		if( pos != agents.end() && pos->m_agent == subscriber )
			agents.erase( pos );

		if( agents.empty() )
			m_subscribers.erase( it );
	}
}

std::string
local_mbox_t::query_name() const
{
	std::ostringstream s;
	s << "<mbox:type=MPMC:id=" << m_id << ">";

	return s.str();
}

void
local_mbox_t::do_deliver_message(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	using namespace so_5::rt::message_limit::impl;

	read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( msg_type );
	if( it != m_subscribers.end() )
		for( auto a : it->second )
			try_to_deliver_to_agent< invocation_type_t::event >(
					*(a.m_agent),
					a.m_limit,
					msg_type,
					message,
					overlimit_reaction_deep,
					[&] {
						agent_t::call_push_event(
								*(a.m_agent),
								a.m_limit,
								m_id,
								msg_type,
								message );
					} );
}

void
local_mbox_t::do_deliver_service_request(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	using namespace so_5::rt::message_limit::impl;

	msg_service_request_base_t::dispatch_wrapper( message,
		[&] {
			read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

			auto it = m_subscribers.find( msg_type );

			if( it == m_subscribers.end() )
				SO_5_THROW_EXCEPTION(
						so_5::rc_no_svc_handlers,
						"no service handlers (no subscribers for message)" );

			if( 1 != it->second.size() )
				SO_5_THROW_EXCEPTION(
						so_5::rc_more_than_one_svc_handler,
						"more than one service handler found" );

			auto & a = it->second.front();
			try_to_deliver_to_agent< invocation_type_t::service_request >(
					*(a.m_agent),
					a.m_limit,
					msg_type,
					message,
					overlimit_reaction_deep,
					[&] {
						agent_t::call_push_service_request(
								*(a.m_agent),
								a.m_limit,
								m_id,
								msg_type,
								message );
					} );
		} );
}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


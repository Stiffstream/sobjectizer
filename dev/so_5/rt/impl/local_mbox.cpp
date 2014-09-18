/*
	SObjectizer 5.
*/

#include <algorithm>
#include <sstream>
#include <mutex>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/impl/h/local_mbox.hpp>

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
	agent_t * subscriber )
{
	std::unique_lock< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( type_wrapper );
	if( it == m_subscribers.end() )
	{
		// There isn't such message type yet.
		agent_container_t container;
		container.push_back( subscriber );

		m_subscribers.emplace( type_wrapper, std::move( container ) );
	}
	else
	{
		auto & agents = it->second;

		auto pos = std::lower_bound( agents.begin(), agents.end(), subscriber );
		if( pos != agents.end() )
		{
			// This is subscriber or appopriate place for it.
			if( *pos != subscriber )
				agents.insert( pos, subscriber );
		}
		else
			agents.emplace_back( subscriber );
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

		auto pos = std::lower_bound( agents.begin(), agents.end(), subscriber );
		if( pos != agents.end() && *pos == subscriber )
			agents.erase( pos );

		if( agents.empty() )
			m_subscribers.erase( it );
	}
}

void
local_mbox_t::deliver_message(
	const std::type_index & type_wrapper,
	const message_ref_t & message_ref ) const
{
	read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( type_wrapper );
	if( it != m_subscribers.end() )
		for( auto a : it->second )
			agent_t::call_push_event(
				*a, m_id, type_wrapper, message_ref );
}

void
local_mbox_t::deliver_service_request(
	const std::type_index & type_index,
	const message_ref_t & svc_request_ref ) const
{
	msg_service_request_base_t & svc_request =
			*(dynamic_cast< msg_service_request_base_t * >(
					svc_request_ref.get() ));

	try
		{
			read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

			auto it = m_subscribers.find( type_index );

			if( it == m_subscribers.end() )
				SO_5_THROW_EXCEPTION(
						so_5::rc_no_svc_handlers,
						"no service handlers (no subscribers for message)" );

			if( 1 != it->second.size() )
				SO_5_THROW_EXCEPTION(
						so_5::rc_more_than_one_svc_handler,
						"more than one service handler found" );

			agent_t::call_push_service_request(
					*(it->second.front()), m_id, type_index, svc_request_ref );
		}
	catch( ... )
		{
			svc_request.set_exception( std::current_exception() );
		}
}

std::string
local_mbox_t::query_name() const
{
	std::ostringstream s;
	s << "<mbox:type=MPMC:id=" << m_id << ">";

	return s.str();
}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


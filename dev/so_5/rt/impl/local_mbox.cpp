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
			if( &(pos->subscriber()) != subscriber )
				agents.insert( pos, info );
			else
				// Agent is already in subscribers list.
				// But its state must be updated.
				pos->set_limit( limit );
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
				subscriber_info_t{ subscriber } );
		if( pos != agents.end() && &(pos->subscriber()) == subscriber )
		{
			// Subscriber can be removed only if there is no delivery filter.
			pos->drop_limit();
			if( pos->empty() )
				agents.erase( pos );
		}

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
		{
			if( a.must_be_delivered( *(message.get()) ) )
				try_to_deliver_to_agent< invocation_type_t::event >(
						a.subscriber(),
						a.limit(),
						msg_type,
						message,
						overlimit_reaction_deep,
						[&] {
							agent_t::call_push_event(
									a.subscriber(),
									a.limit(),
									m_id,
									msg_type,
									message );
						} );
		}
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

			const auto & svc_request_param =
				dynamic_cast< msg_service_request_base_t & >( *message )
						.query_param();

			auto & a = it->second.front();
			if( a.must_be_delivered( svc_request_param ) )
				try_to_deliver_to_agent< invocation_type_t::service_request >(
						a.subscriber(),
						a.limit(),
						msg_type,
						message,
						overlimit_reaction_deep,
						[&] {
							agent_t::call_push_service_request(
									a.subscriber(),
									a.limit(),
									m_id,
									msg_type,
									message );
						} );
			else
				SO_5_THROW_EXCEPTION(
						so_5::rc_no_svc_handlers,
						"no service handlers (no subscribers for message or "
						"subscriber is blocked by delivery filter)" );
		} );
}

void
local_mbox_t::set_delivery_filter(
	const std::type_index & msg_type,
	const delivery_filter_t & filter,
	agent_t & subscriber )
{
	std::unique_lock< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( msg_type );
	if( it == m_subscribers.end() )
	{
		// There isn't such message type yet.
		subscriber_container_t container;
		container.emplace_back( &subscriber, &filter );

		m_subscribers.emplace( msg_type, std::move( container ) );
	}
	else
	{
		auto & agents = it->second;

		subscriber_info_t info{ &subscriber, &filter };

		auto pos = std::lower_bound( agents.begin(), agents.end(), info );
		if( pos != agents.end() )
		{
			// This is subscriber or appopriate place for it.
			if( &(pos->subscriber()) != &subscriber )
				agents.insert( pos, info );
			else
				// Agent is already in subscribers list.
				// But its state must be updated.
				pos->set_filter( filter );
		}
		else
			agents.push_back( info );
	}
}

void
local_mbox_t::drop_delivery_filter(
	const std::type_index & msg_type,
	agent_t & subscriber ) SO_5_NOEXCEPT
{
	std::unique_lock< default_rw_spinlock_t > lock( m_lock );

	auto it = m_subscribers.find( msg_type );
	if( it != m_subscribers.end() )
	{
		auto & agents = it->second;

		auto pos = std::lower_bound( agents.begin(), agents.end(),
				subscriber_info_t{ &subscriber } );
		if( pos != agents.end() && &(pos->subscriber()) == &subscriber )
		{
			// Subscriber can be removed only if there is no delivery filter.
			pos->drop_filter();
			if( pos->empty() )
				// There is no more need in that subscriber.
				agents.erase( pos );
		}

		if( agents.empty() )
			m_subscribers.erase( it );
	}
}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


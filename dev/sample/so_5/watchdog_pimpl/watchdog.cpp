#include "watchdog.hpp"

// Useful short typename.
using op_id_t = unsigned long long;

namespace
{

// Message to start watching operation.
struct msg_start : public so_5::rt::message_t
{
	msg_start(
		std::string tag,
		const std::chrono::steady_clock::duration & timeout )
		:	m_tag( std::move( tag ) )
		,	m_timeout( timeout )
	{}

	const std::string m_tag;
	const std::chrono::steady_clock::duration m_timeout;
};

// Message to stop watching operation.
struct msg_stop : public so_5::rt::message_t
{
	msg_stop( std::string tag )
		:	m_tag( std::move( tag ) )
	{}

	const std::string m_tag;
};

// Message to inform about operation timeout.
struct msg_timeout : public so_5::rt::message_t
{
	msg_timeout(
		std::string tag,
		op_id_t id )
		:	m_tag( std::move( tag ) )
		,	m_id( id )
	{}

	const std::string m_tag;
	op_id_t m_id;
};

} /* namespace anonymous */

//
// operation_watchdog_t
//

operation_watchdog_t::operation_watchdog_t(
	const watchdog_t & watchdog,
	std::string tag,
	const std::chrono::steady_clock::duration & timeout )
	:	m_watchdog( watchdog )
	,	m_tag( std::move( tag ) )
{
	m_watchdog.start( m_tag, timeout );
}

operation_watchdog_t::~operation_watchdog_t()
{
	m_watchdog.stop( m_tag );
}

//
// watchdog_t
//

watchdog_t::watchdog_t( const so_5::rt::mbox_t & mbox )
	:	m_mbox( mbox )
{
}

watchdog_t::~watchdog_t()
{
}

void
watchdog_t::start(
	const std::string & tag,
	const std::chrono::steady_clock::duration & timeout ) const
{
	so_5::send< msg_start >( m_mbox, tag, timeout );
}

void
watchdog_t::stop( const std::string & tag ) const
{
	so_5::send< msg_stop >( m_mbox, tag );
}

// Private implementation of watchdog agent.
class a_watchdog_impl_t
{
	public:
		a_watchdog_impl_t( so_5::rt::agent_t & a_watchdog );

		//
		// Message handlers.
		//

		void
		handle( const msg_start & m );

		void
		handle( const msg_stop & m );

		void
		handle( const msg_timeout & m );

	private:
		// An agent for delayed messages.
		so_5::rt::agent_t & m_watchdog_agent;

		// This counter will be used for ID generation.
		op_id_t m_id_base = 0;

		// Information about one specific operation.
		struct details_t
		{
			details_t(
				op_id_t id,
				so_5::timer_id_t timer )
				:	m_id( id )
				,	m_timer( std::move( timer ) )
			{}

			op_id_t m_id;
			so_5::timer_id_t m_timer;
		};

		// Type of map from operation tag name to operation details.
		using operation_map_t = std::map< std::string, details_t >;

		// Info about watched operations.
		operation_map_t m_operations;
};

a_watchdog_impl_t::a_watchdog_impl_t(
	so_5::rt::agent_t & a_watchdog )
	:	m_watchdog_agent( a_watchdog )
{}

void
a_watchdog_impl_t::handle( const msg_start & m )
{
	auto it = m_operations.find( m.m_tag );
	if( it == m_operations.end() )
	{
		auto id = m_id_base + 1;
		auto timer = so_5::send_periodic_to_agent< msg_timeout >(
				m_watchdog_agent,
				m.m_timeout, std::chrono::steady_clock::duration::zero(),
				m.m_tag, id );

		m_operations.emplace(
				operation_map_t::value_type{
						m.m_tag,
						details_t{ id, timer }
				} );
		m_id_base = id;
	}
	else
	{
		std::cerr << "Operation with tag {" << m.m_tag << "} "
				"is already watched. "
				"Note that duplicate operation will be unwatched.";
	}
}

void
a_watchdog_impl_t::handle( const msg_stop & m )
{
	m_operations.erase( m.m_tag );
}

void
a_watchdog_impl_t::handle( const msg_timeout & m )
{
	auto it = m_operations.find( m.m_tag );
	if( it != m_operations.end() )
		if( it->second.m_id == m.m_id )
		{
			std::cerr << "Operation with tag {" << m.m_tag << "} timedout."
					<< std::endl
					<< "Watchdog calls application to abort."
					<< std::endl;
			std::abort();
		}
}

//
// a_watchdog_t
//

a_watchdog_t::a_watchdog_t(
	so_5::rt::environment_t & env )
	:	so_5::rt::agent_t( env )
	,	m_impl( new a_watchdog_impl_t( *self_ptr() ) )
{}

a_watchdog_t::~a_watchdog_t()
{}

void
a_watchdog_t::so_define_agent()
{
	// Implement all internal event logic using lamda-handlers.

	// Each event is delegated to impl.
	so_default_state()
		.event([&]( const msg_start & m ){ m_impl->handle( m ); } )
		.event([&]( const msg_stop & m ){ m_impl->handle( m ); } )
		.event([&]( const msg_timeout & m ){ m_impl->handle( m ); } );
}

watchdog_t
a_watchdog_t::create_watchdog() const
{
	return watchdog_t( so_direct_mbox() );
}


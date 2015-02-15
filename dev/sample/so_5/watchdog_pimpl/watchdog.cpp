#include "watchdog.hpp"

namespace /* anonymous */
{

// Signal for periodic check of watched operations.
struct msg_check : public so_5::rt::signal_t {};

// Message to start watching operation.
struct msg_start_watch : public so_5::rt::message_t
{
	msg_start_watch(
		std::string operation_tag,
		const std::chrono::steady_clock::duration & timeout )
		:	m_operation_tag( std::move( operation_tag ) )
		,	m_timeout( timeout )
	{}

	const std::string m_operation_tag;
	const std::chrono::steady_clock::duration m_timeout;
};

// Message to stop watching operation.
struct msg_stop_watch : public so_5::rt::message_t
{
	msg_stop_watch( std::string operation_tag )
		:	m_operation_tag( std::move( operation_tag ) )
	{}

	const std::string m_operation_tag;
};

} /* namespace anonymous */

//
// operation_watchdog_t
//

operation_watchdog_t::operation_watchdog_t(
	const watchdog_t & watchdog,
	std::string operation_tag,
	const std::chrono::steady_clock::duration & timeout )
	:	m_watchdog( watchdog )
	,	m_operation_tag( std::move( operation_tag ) )
{
	m_watchdog.start_watch_operation( m_operation_tag, timeout );
}

operation_watchdog_t::~operation_watchdog_t()
{
	m_watchdog.stop_watch_operation( m_operation_tag );
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
watchdog_t::start_watch_operation(
	const std::string & operation_tag,
	const std::chrono::steady_clock::duration & timeout ) const
{
	so_5::send< msg_start_watch >( m_mbox,
			operation_tag, timeout );
}

void
watchdog_t::stop_watch_operation( const std::string & operation_tag ) const
{
	so_5::send< msg_stop_watch >( m_mbox, operation_tag );
}

// Private implementation of watchdog agent.
class a_watchdog_impl_t
{
	public:
		a_watchdog_impl_t(
			const std::chrono::steady_clock::duration & check_interval )
			:	m_check_interval( check_interval )
		{}

		void
		evt_start( so_5::rt::agent_t & agent );

		// Check timedout operations.
		void
		check();

		//
		// Message handlers.
		//

		void
		handle( const msg_start_watch & m );

		void
		handle( const msg_stop_watch & m );

	private:
		const std::chrono::steady_clock::duration m_check_interval;
		so_5::timer_id_t m_check_timer;

		// Info about watched operations.
		using operation_map_t =
				std::map< std::string, std::chrono::steady_clock::time_point >;
		operation_map_t m_watched_operations;
};

void
a_watchdog_impl_t::evt_start( so_5::rt::agent_t & agent )
{
	m_check_timer = so_5::send_periodic_to_agent< msg_check >(
			agent,
			m_check_interval,
			m_check_interval );
}

void
a_watchdog_impl_t::check()
{
	bool abort_needed = false;

	const auto now = std::chrono::steady_clock::now();

	for( const auto & op : m_watched_operations )
	{
		if( now > op.second )
		{
			std::cerr << "Operation with tag {" << op.first << "} timedout.";
			abort_needed = true;
		}
	}

	if( abort_needed )
	{
		std::cerr << "Watchdog calls application to abort.";
		std::abort();
	}
}

void
a_watchdog_impl_t::handle( const msg_start_watch & m )
{
	auto r = m_watched_operations.emplace(
			operation_map_t::value_type(
					m.m_operation_tag,
					std::chrono::steady_clock::now() + m.m_timeout ) );

	if( !r.second )
	{
		std::cerr << "Operation with tag {" << m.m_operation_tag << "} "
				"is already watched. "
				"Note that duplicate operation will be unwatched.";
	}
}

void
a_watchdog_impl_t::handle( const msg_stop_watch & m )
{
	m_watched_operations.erase( m.m_operation_tag );
}

//
// a_watchdog_t
//

a_watchdog_t::a_watchdog_t(
	so_5::rt::environment_t & env,
	const std::chrono::steady_clock::duration & check_interval )
	:	so_5::rt::agent_t( env )
	,	m_impl( new a_watchdog_impl_t( check_interval ) )
{}

a_watchdog_t::~a_watchdog_t()
{}

void
a_watchdog_t::so_define_agent()
{
	// Implement all internal event logic using lamda-handlers.

	// Each event is delegated to impl.
	so_subscribe_self()
		.event< msg_check >( [&]{ m_impl->check(); } )
		.event([&]( const msg_start_watch & m ){ m_impl->handle( m ); } )
		.event([&]( const msg_stop_watch & m ){ m_impl->handle( m ); } );
}

void
a_watchdog_t::so_evt_start()
{
	m_impl->evt_start( *this );
}

watchdog_t
a_watchdog_t::create_watchdog() const
{
	return watchdog_t( so_direct_mbox() );
}


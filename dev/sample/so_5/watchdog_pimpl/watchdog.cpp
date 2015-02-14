#include <sample/so_5/watchdog_pimpl/h/watchdog.hpp>

namespace /* anonymous */
{

// Signal for periodic check of watched operations.
struct check_watched_operations_t : public so_5::rt::signal_t {};

// Message to start watching operation.
struct start_watch_operation_t
	:
		public so_5::rt::message_t
{
	start_watch_operation_t(
		const std::string & operation_tag,
		const std::chrono::steady_clock::duration & timeout )
		:
			m_operation_tag( operation_tag ),
			m_timeout( timeout )
	{}

	const std::string m_operation_tag;
	const std::chrono::steady_clock::duration m_timeout;
};

// Message to stop watching operation.
struct stop_watch_operation_t
	:
		public so_5::rt::message_t
{
	stop_watch_operation_t(
		const std::string & operation_tag )
		:
			m_operation_tag( operation_tag )
	{}

	const std::string m_operation_tag;
};

}

//
// operation_watchdog_t
//

operation_watchdog_t::operation_watchdog_t(
	const watchdog_t & watchdog,
	const std::string & operation_tag,
	const std::chrono::steady_clock::duration & timeout )
	:
		m_watchdog( watchdog ),
		m_operation_tag( operation_tag )
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
	:
		m_mbox( mbox )
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
	m_mbox->deliver_message(
		new start_watch_operation_t( operation_tag, timeout ) );
}

void
watchdog_t::stop_watch_operation( const std::string & operation_tag ) const
{
	m_mbox->deliver_message(
		new stop_watch_operation_t( operation_tag ) );
}

// Private implementation of watchdog agent.
class a_watchdog_impl_t
{
	public:
		a_watchdog_impl_t(
			const std::chrono::steady_clock::duration & check_interval,
			so_5::error_logger_t & error_logger )
			:
				m_check_interval( check_interval ),
				m_error_logger( error_logger )
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
		handle(
			const start_watch_operation_t & m );

		void
		handle(
			const stop_watch_operation_t & m );

	private:
		const std::chrono::steady_clock::duration m_check_interval;
		so_5::timer_id_t m_check_timer;

		so_5::error_logger_t & m_error_logger;

		// Info about watched operations.
		std::map< std::string, std::chrono::steady_clock::time_point >
			m_watched_operations;
};

void
a_watchdog_impl_t::evt_start( so_5::rt::agent_t & agent )
{
	m_check_timer = so_5::send_periodic_to_agent< check_watched_operations_t >(
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
			SO_5_LOG_ERROR( m_error_logger, errstream )
			{
				errstream
					<< "Operation with tag {" << op.first << "} timedout.";
			}
			abort_needed = true;
		}
	}

	if( abort_needed )
	{
		SO_5_LOG_ERROR( m_error_logger, errstream )
		{
			errstream
				<< "Watchdog calls application to abort.";
		}

		std::abort();
	}
}

void
a_watchdog_impl_t::handle(
	const start_watch_operation_t & m )
{
	auto it = m_watched_operations.find( m.m_operation_tag );

	if( m_watched_operations.end() != it )
	{
		SO_5_LOG_ERROR( m_error_logger, errstream )
		{
			errstream << "Operation with tag {" << m.m_operation_tag << "} "
				"is already watched. "
				"Note that duplicate operation will be unwatched.";
		}
	}
	else
	{
		m_watched_operations[ m.m_operation_tag ] =
			std::chrono::steady_clock::now() + m.m_timeout;
	}
}

void
a_watchdog_impl_t::handle(
	const stop_watch_operation_t & m )
{
	m_watched_operations.erase( m.m_operation_tag );
}

//
// a_watchdog_t
//

a_watchdog_t::a_watchdog_t(
	so_5::rt::environment_t & env,
	const std::chrono::steady_clock::duration & check_interval )
	:
		base_type_t( env ),
		m_watchdog_impl(
			new a_watchdog_impl_t(
				check_interval,
				env.error_logger() ) )
{
}

a_watchdog_t::~a_watchdog_t()
{
}

void
a_watchdog_t::so_define_agent()
{
	// Implement all internal event logic using lamda-handlers.

	// Each event is delegated to impl.

	so_subscribe_self()
		.event< check_watched_operations_t >( [&](){
			m_watchdog_impl->check();
			} )
		.event([&]( const start_watch_operation_t & m ){
			m_watchdog_impl->handle( m );
			} )
		.event([&]( const stop_watch_operation_t & m ){
			m_watchdog_impl->handle( m );
			} );
}

void
a_watchdog_t::so_evt_start()
{
	m_watchdog_impl->evt_start( *this );
}

watchdog_t
a_watchdog_t::create_watchdog() const
{
	return watchdog_t( so_direct_mbox() );
}

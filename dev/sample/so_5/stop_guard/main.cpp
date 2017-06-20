/*
 * A sample of using stop_guard.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of stop_guard for the example.
// This guard sends a shutdown_started signal to the specified mbox.
class example_guard final
	: public so_5::stop_guard_t
	, public std::enable_shared_from_this< example_guard >
{
public :
	// A signal to be sent when shutdown is started.
	struct shutdown_started final : public so_5::signal_t {};

	example_guard(
		// Mbox to which shutdown_started must be sent.
		so_5::mbox_t dest ) : m_dest( std::move(dest) )
	{}

	virtual void
	stop() SO_5_NOEXCEPT override
	{
		so_5::send< shutdown_started >( m_dest );
	}

private :
	const so_5::mbox_t m_dest;
};

// Definition of an agent for SObjectizer.
class worker final : public so_5::agent_t
{
	// Worker will have two states:
	state_t st_normal{ this }, st_shutdown{ this };

	// These signals will be used by worker.
	struct timer final : public so_5::signal_t {};
	struct terminate_work final : public so_5::signal_t {};

public:
	worker( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		this >>= st_normal;

		st_normal
			.event( &worker::on_timer_normal )
			.event( &worker::on_shutdown_started );

		st_shutdown
			.event( &worker::on_timer_shutdown )
			.event( &worker::on_terminate );
	}

	// A reaction to start of work in SObjectizer.
	virtual void so_evt_start() override
	{
		// Stop_guard must be created and installed.
		m_guard = std::make_shared< example_guard >( so_direct_mbox() );
		so_environment().setup_stop_guard( m_guard );

		// Periodic timer message must be sent.
		m_timer = so_5::send_periodic< timer >( *this,
				std::chrono::milliseconds(125),
				std::chrono::milliseconds(125) );
	}

private :
	// Stop_guard to be used by this agent.
	so_5::stop_guard_shptr_t m_guard;
	// Timer ID for periodic message.
	so_5::timer_id_t m_timer;

	void on_timer_normal( mhood_t<timer> )
	{
		std::cout << "working in normal mode..." << std::endl;
	}

	void on_timer_shutdown( mhood_t<timer> )
	{
		std::cout << "working in shutdown mode..." << std::endl;
	}

	void on_shutdown_started( mhood_t<example_guard::shutdown_started> ) 
	{
		std::cout << "shutdown is in progress!" << std::endl;

		this >>= st_shutdown;
		so_5::send_delayed< terminate_work >( *this,
				std::chrono::milliseconds(300) );
	}

	void on_terminate( mhood_t<terminate_work> )
	{
		so_environment().remove_stop_guard( m_guard );

		std::cout << "terminate work." << std::endl;
	}
};

// This agent will initiate the stop operation.
class work_stopper final : public so_5::agent_t
{
	struct stop_work final : public so_5::signal_t {};

public :
	work_stopper( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( [this]( mhood_t<stop_work> ) {
			// Initiate shutdown of SObjectizer Environment.
			so_environment().stop();
		} );
	}

	virtual void so_evt_start() override
	{
		so_5::send_delayed< stop_work >( *this,
				std::chrono::milliseconds(400) );
	}
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Create coop with example's agents.
				env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.make_agent< worker >();
					coop.make_agent< work_stopper >();
				} );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

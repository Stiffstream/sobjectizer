/*
 * A sample of a simple agent which sends a periodic message to itself.
 */

#include <iostream>
#include <time.h>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Hello message.
struct msg_hello_periodic : public so_5::rt::message_t
{
	// Greeting.
	std::string m_message;
};

// Stop message.
class msg_stop_signal : public so_5::rt::signal_t {};

// An agent class definition.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t( so_5::rt::so_environment_t & env )
			:	so_5::rt::agent_t( env )
			,	m_evt_count( 0 )
			,	m_shutdowner_mbox(
					so_environment().create_local_mbox( "shutdown" ) )
		{}
		virtual ~a_hello_t()
		{}

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent();

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start();

		// Hello message handler.
		void
		evt_hello_periodic( const msg_hello_periodic & msg );

	private:
		// Agent's mbox.
		const so_5::rt::mbox_ref_t m_shutdowner_mbox;

		// Timer events' identifiers.
		so_5::timer_thread::timer_id_ref_t m_hello_timer_id;
		so_5::timer_thread::timer_id_ref_t m_stop_timer_id;

		// How much timer event has been processed.
		unsigned int m_evt_count;
};

void
a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe( so_direct_mbox() )
		.event( &a_hello_t::evt_hello_periodic );
}

void
a_hello_t::so_evt_start()
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) )
		<< "a_hello_t::so_evt_start()" << std::endl;

	std::unique_ptr< msg_hello_periodic > msg( new msg_hello_periodic );
	msg->m_message = "Hello, periodic!";

	// Sending a greeting.
	m_hello_timer_id =
		so_environment()
			.schedule_timer(
				std::move( msg ),
				so_direct_mbox(),
				// Delay for a second.
				so_5::chrono_helpers::to_ms( std::chrono::seconds(1) ),
				// Repeat every 1.25 of seconds.
				so_5::chrono_helpers::to_ms( std::chrono::milliseconds(1250) ) );

	// Sending a stop signal.
	m_stop_timer_id =
		so_environment()
			.schedule_timer< msg_stop_signal >(
				m_shutdowner_mbox,
				// Delay for two seconds.
				so_5::chrono_helpers::to_ms( std::chrono::seconds(2) ),
				// Not a periodic.
				0 );
}

void
a_hello_t::evt_hello_periodic( const msg_hello_periodic & msg )
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) )
		<< msg.m_message << std::endl;

	if( 5 == ++m_evt_count )
	{
		// Stops hello message.
		m_hello_timer_id.release();
	}
	else
	{
		// Reschedule a stop signal.
		// Previous a stop signal should be canceled.
		m_stop_timer_id =
			so_environment()
				.schedule_timer< msg_stop_signal >(
					m_shutdowner_mbox,
					// 1300ms but specified in microsecs just for demonstration.
					so_5::chrono_helpers::to_ms( std::chrono::microseconds(1300000) ),
					0 );
	}
}

// Creation of 'hello' cooperation.
void
create_hello_coop( so_5::rt::so_environment_t & env )
{
	// Single agent can be registered as whole cooperation.
	env.register_agent_as_coop( "hello", new a_hello_t( env ) );
}

// Creation of 'shutdowner' cooperation.
void
create_shutdowner_coop( so_5::rt::so_environment_t & env )
{
	// Mbox for shutdowner agent.
	auto mbox = env.create_local_mbox( "shutdown" );

	// Cooperation for shutdowner.
	auto coop = env.create_coop( "shutdowner" );
	
	// Shutdowner agent.
	coop->define_agent()
		.event( mbox, so_5::signal< msg_stop_signal >,
			[&env, mbox]() {
				time_t t = time( 0 );
				std::cout << asctime( localtime( &t ) )
					<< "Stop SObjectizer..." << std::endl;

				// Shutting down SObjectizer.
				env.stop();
			} );

	env.register_coop( std::move( coop ) );
}

// The SObjectizer Environment initialization.
void
init( so_5::rt::so_environment_t & env )
{
	create_hello_coop( env );
	create_shutdowner_coop( env );
}

int
main( int, char ** )
{
	try
	{
		so_5::api::run_so_environment( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


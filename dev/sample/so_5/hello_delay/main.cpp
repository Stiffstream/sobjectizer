/*
 * A sample of a simple agent which send a delayed message to itself.
 */

#include <iostream>
#include <time.h>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Hello message.
struct msg_hello : public so_5::rt::message_t
{
	// Greeting.
	std::string m_message;
};

// Stop message.
class msg_stop_signal : public so_5::rt::signal_t {};

// An agent class.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t( so_5::rt::environment_t & env )
			: so_5::rt::agent_t( env )
		{}
		virtual ~a_hello_t()
		{}

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent();

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start();

		// Delayed message handler.
		void
		evt_hello_delay( const msg_hello & msg );

		// Stop signal handler.
		void
		evt_stop_signal();
};

void
a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe( so_direct_mbox() )
		.event( &a_hello_t::evt_hello_delay );

	so_subscribe( so_direct_mbox() )
		.event( so_5::signal< msg_stop_signal >, &a_hello_t::evt_stop_signal );
}

void
a_hello_t::so_evt_start()
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) )
		<< "a_hello_t::so_evt_start()" << std::endl;

	std::unique_ptr< msg_hello > msg( new msg_hello );
	msg->m_message = "Hello, world! This is SObjectizer v.5.";

	// Send greeting.
	so_environment().single_timer(
		std::move( msg ),
		so_direct_mbox(),
		// Delay for 2 seconds.
		2*1000 );
}

void
a_hello_t::evt_hello_delay( const msg_hello & msg )
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) ) << msg.m_message << std::endl;

	so_environment().single_timer< msg_stop_signal >(
		so_direct_mbox(),
		2*1000 );
}

void
a_hello_t::evt_stop_signal()
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) )
		<< "Stop SObjectizer..." << std::endl;

	// Shutting down SObjectizer.
	so_environment().stop();
}

int
main( int, char ** )
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
				env.register_agent_as_coop( "coop", new a_hello_t( env ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


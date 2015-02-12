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

	msg_hello( std::string message ) : m_message( std::move( message ) ) {}
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

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override;

		// Delayed message handler.
		void
		evt_hello_delay( const msg_hello & msg );

		// Stop signal handler.
		void
		evt_stop_signal();

	private:
		// Helper function for printing messages.
		static void
		show_message( const std::string & what );
};

void
a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe_self().event( &a_hello_t::evt_hello_delay );

	so_subscribe_self().event< msg_stop_signal >( &a_hello_t::evt_stop_signal );
}

void
a_hello_t::so_evt_start()
{
	show_message( "a_hello_t::so_evt_start()" );

	so_5::send_delayed_to_agent< msg_hello >(
		*this,
		std::chrono::seconds( 2 ),
		"Hello, world! This is SObjectizer v.5." );
}

void
a_hello_t::evt_hello_delay( const msg_hello & msg )
{
	show_message( msg.m_message );

	so_5::send_delayed_to_agent< msg_stop_signal >(
		*this,
		std::chrono::seconds( 2 ) );
}

void
a_hello_t::evt_stop_signal()
{
	show_message( "Stop SObjectizer..." );

	// Shutting down SObjectizer.
	so_environment().stop();
}

void
a_hello_t::show_message( const std::string & what )
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) ) << what << std::endl;
}

int
main()
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


/*
 * A sample of a simple agent which send a delayed message to itself.
 */

#if defined( _MSC_VER )
	#if defined( __clang__ )
		#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#endif
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <ctime>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Hello message.
struct msg_hello
{
	// Greeting.
	std::string m_message;
};

// Stop message.
class msg_stop_signal : public so_5::signal_t {};

// An agent class.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx ) : so_5::agent_t( ctx )
		{}

		// Definition of an agent for SObjectizer.
		virtual void so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override;

		// Delayed message handler.
		void evt_hello_delay( const msg_hello & msg );

		// Stop signal handler.
		void evt_stop_signal();

	private:
		// Helper function for printing messages.
		static void show_message( const std::string & what );
};

void a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe_self().event( &a_hello_t::evt_hello_delay );

	so_subscribe_self().event< msg_stop_signal >( &a_hello_t::evt_stop_signal );
}

void a_hello_t::so_evt_start()
{
	show_message( "a_hello_t::so_evt_start()" );

	so_5::send_delayed< msg_hello >(
		*this,
		std::chrono::seconds( 2 ),
		"Hello, world! This is SObjectizer v.5." );
}

void a_hello_t::evt_hello_delay( const msg_hello & msg )
{
	show_message( msg.m_message );

	so_5::send_delayed< msg_stop_signal >(
		*this,
		std::chrono::seconds( 2 ) );
}

void a_hello_t::evt_stop_signal()
{
	show_message( "Stop SObjectizer..." );

	// Shutting down SObjectizer.
	so_environment().stop();
}

void a_hello_t::show_message( const std::string & what )
{
	time_t t = time( nullptr );
	std::cout << std::asctime( std::localtime( &t ) ) << what << std::endl;
}

int main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( "coop", env.make_agent< a_hello_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


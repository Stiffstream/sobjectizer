/*
 * A sample of a simple agent which sends a periodic message to itself.
 */

#if defined( _MSC_VER )
	#if defined( __clang__ )
		#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#endif
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <time.h>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Hello message.
struct msg_hello_periodic
{
	// Greeting.
	std::string m_message;
};

// Stop message.
class msg_stop_signal : public so_5::signal_t {};

// An agent class definition.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx )
			:	so_5::agent_t{ ctx }
			,	m_shutdowner_mbox( so_environment().create_mbox( "shutdown" ) )
			,	m_evt_count( 0 )
		{}

		// Definition of an agent for SObjectizer.
		virtual void so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override;

	private:
		// Agent's mbox.
		const so_5::mbox_t m_shutdowner_mbox;

		// Timer events' identifiers.
		so_5::timer_id_t m_hello_timer_id;
		so_5::timer_id_t m_stop_timer_id;

		// How much timer event has been processed.
		unsigned int m_evt_count;

		// Hello message handler.
		void evt_hello_periodic( const msg_hello_periodic & msg );
};

void a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe_self().event( &a_hello_t::evt_hello_periodic );
}

void a_hello_t::so_evt_start()
{
	time_t t = time( nullptr );
	std::cout << asctime( localtime( &t ) )
		<< "a_hello_t::so_evt_start()" << std::endl;

	// Sending a greeting.
	m_hello_timer_id = so_5::send_periodic< msg_hello_periodic >(
		*this,
		// Delay for a second.
		std::chrono::seconds(1),
		// Repeat every 1.25 of seconds.
		std::chrono::milliseconds(1250),
		"Hello, periodic!" );

	// Sending a stop signal.
	m_stop_timer_id = so_5::send_periodic< msg_stop_signal >(
		so_environment(),
		m_shutdowner_mbox,
		// Delay for two seconds.
		std::chrono::seconds(2),
		// Not a periodic.
		std::chrono::seconds::zero() );
}

void a_hello_t::evt_hello_periodic( const msg_hello_periodic & msg )
{
	time_t t = time( nullptr );
	std::cout << asctime( localtime( &t ) ) << msg.m_message << std::endl;

	if( 5 == ++m_evt_count )
	{
		// Stops hello message.
		m_hello_timer_id.release();
	}
	else
	{
		// Reschedule a stop signal.
		// Previous a stop signal should be canceled.
		m_stop_timer_id = so_5::send_periodic< msg_stop_signal >(
			so_environment(),
			m_shutdowner_mbox,
			// 1300ms but specified in microsecs just for demonstration.
			std::chrono::microseconds(1300000),
			std::chrono::seconds::zero() );
	}
}

// Creation of 'hello' cooperation.
void create_hello_coop( so_5::environment_t & env )
{
	// Single agent can be registered as whole cooperation.
	env.register_agent_as_coop( "hello", env.make_agent< a_hello_t >() );
}

// Creation of 'shutdowner' cooperation.
void create_shutdowner_coop( so_5::environment_t & env )
{
	env.introduce_coop( "shutdowner", [&env]( so_5::coop_t & coop ) {
		// Mbox for shutdowner agent.
		auto mbox = env.create_mbox( "shutdown" );

		// Shutdowner agent.
		coop.define_agent().event< msg_stop_signal >( mbox,
			[&env, mbox]() {
				time_t t = time( nullptr );
				std::cout << asctime( localtime( &t ) )
					<< "Stop SObjectizer..." << std::endl;

				// Shutting down SObjectizer.
				env.stop();
			} );
	});
}

// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	create_hello_coop( env );
	create_shutdowner_coop( env );
}

int main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


/*
 * A sample of using mbox for subscribing messages.
 */

#include <iostream>
#include <string>

// Main SObjectizer header files.
#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

// State sequence for sample agent.
enum sample_state_t
{
	DEFAULT_STATE,
	FIRST_STATE,
	SECOND_STATE,
};

// Message for the changing state.
struct change_state_message
	:
		public so_5::rt::message_t
{
	// Helper factory for creating ready to send messages.
	static std::unique_ptr< change_state_message >
	create( sample_state_t state )
	{
		return std::unique_ptr< change_state_message >(
			 new change_state_message( state ) );
	}

	change_state_message(
		sample_state_t next_state )
		:
			m_next_state( next_state )
	{}

	const sample_state_t m_next_state;
};

// Sample message.
struct my_message
	:
		public so_5::rt::message_t
{
	// Helper factory for creating ready to send messages.
	static std::unique_ptr< my_message >
	create( int x )
	{
		std::unique_ptr< my_message > result( new my_message );

		result->x = x;

		return result;
	}

	// Some data.
	int x;
};

// Another sample message.
struct my_another_message
	:
		public so_5::rt::message_t
{
	// Helper factory for creating ready to send messages.
	static std::unique_ptr< my_another_message >
	create( const std::string & s )
	{
		std::unique_ptr< my_another_message > result( new my_another_message );

		result->s = s;

		return result;
	}

	// Some data.
	std::string s;
};

// Sample message for the subscription demonstrtion.
class my_agent_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		my_agent_t(
			so_5::rt::so_environment_t & env );

		virtual ~my_agent_t() {}

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent();

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start();

		// Handle change state.
		void
		change_state_event_handler(
			const change_state_message & message );

		// Handle my_message.
		void
		my_event_handler(
			const my_message & message );

		// Handle my_another_message.
		void
		my_another_event_handler(
			const my_another_message & message );

	private:
		// Agent states.
		const so_5::rt::state_t m_first_state = so_make_state( "first" );
		const so_5::rt::state_t m_second_state = so_make_state( "second" );
};

my_agent_t::my_agent_t(
	so_5::rt::so_environment_t & env )
	:
		base_type_t( env )
{
}

void
my_agent_t::so_define_agent()
{
	std::cout << "so_define_agent()" << std::endl;

	so_subscribe( so_direct_mbox() )
		.in( m_first_state )
		.event( &my_agent_t::change_state_event_handler );
	so_subscribe( so_direct_mbox() )
		.in( m_second_state )
		.event( &my_agent_t::change_state_event_handler );
	so_subscribe( so_direct_mbox() )
		.event( &my_agent_t::change_state_event_handler );

	std::cout << "\tsubscribe my_event_handler in "
		<< m_first_state.query_name()
		<< std::endl;

	so_subscribe( so_direct_mbox() )
		.in( m_first_state )
		.event( &my_agent_t::my_event_handler );

	std::cout << "\tsubscribe my_another_event_handler in "
		<< m_first_state.query_name()
		<< std::endl;

	so_subscribe( so_direct_mbox() )
		.in( m_first_state )
		.event( &my_agent_t::my_another_event_handler );

	std::cout << "\tsubscribe my_event_handler in "
		<< m_second_state.query_name()
		<< std::endl;

	so_subscribe( so_direct_mbox() )
		.in( m_second_state )
		.event( &my_agent_t::my_event_handler );
}

void
my_agent_t::so_evt_start()
{
	std::cout << "so_evt_start()" << std::endl;

	std::cout << "\tsend sample messages sequence for state changes" << std::endl;
	// Send siries of messages.

	// Switch to first state and handle messages.
	so_direct_mbox()->deliver_message( change_state_message::create( FIRST_STATE ) );
}

void
my_agent_t::change_state_event_handler(
	const change_state_message & message )
{
	std::cout << "change_state_event_handler()" << std::endl;

	if( DEFAULT_STATE == message.m_next_state )
	{
		so_change_state( so_default_state() );
		std::cout << "\tswitch to default state and shutdown..." << std::endl;
		so_environment().stop();
	}
	else
	{
		if( FIRST_STATE == message.m_next_state )
		{
			so_change_state( m_first_state );

			std::cout << "\tswitch to " << so_current_state().query_name()
				<< std::endl;

			so_direct_mbox()->deliver_message( my_message::create( 42 ) );
			so_direct_mbox()->deliver_message( my_another_message::create( "SObjectizer" ) );

			std::cout << "\tmessages sent" << std::endl;

			// Switch to second.
			so_direct_mbox()->deliver_message( change_state_message::create( SECOND_STATE ) );
		}
		else if( SECOND_STATE == message.m_next_state )
		{
			so_change_state( m_second_state );

			std::cout << "\tswitch to "
				<< so_current_state().query_name()
				<< std::endl;

			so_direct_mbox()->deliver_message( my_message::create( -42 ) );
			// Message should not be received.
			so_direct_mbox()->deliver_message( my_another_message::create( "rezitcejbOS" ) );

			std::cout << "\tmessages sent" << std::endl;

			// Switch to default.
			so_direct_mbox()->deliver_message( change_state_message::create( DEFAULT_STATE ) );
		}
	}
}

void
my_agent_t::my_event_handler(
	const my_message & message )
{
	std::cout << "my_event_handler()" << std::endl;

	std::cout
		<< "\tcurrent state is " << so_current_state().query_name() << std::endl
		<< "\tmessage.x = " << message.x << std::endl;
}

void
my_agent_t::my_another_event_handler(
	const my_another_message & message )
{
	std::cout << "my_another_event_handler()" << std::endl;

	std::cout
		<< "\tcurrent state is " << so_current_state().query_name() << std::endl
		<< "\tmessage.s = " << message.s << std::endl;
}


int
main( int, char ** )
{
	try
	{
		so_5::api::run_so_environment(
			[]( so_5::rt::so_environment_t & env )
			{
				env.register_agent_as_coop( "coop", new my_agent_t( env ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


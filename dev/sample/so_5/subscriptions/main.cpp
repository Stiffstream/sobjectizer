/*
 * A sample of using mbox for subscribing messages.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// State sequence for sample agent.
enum sample_state_t
{
	DEFAULT_STATE,
	FIRST_STATE,
	SECOND_STATE,
};

// Message for the changing state.
struct change_state_message : public so_5::rt::message_t
{
	change_state_message( sample_state_t next_state )
		: m_next_state( next_state )
	{}

	const sample_state_t m_next_state;
};

// Sample message.
struct my_message : public so_5::rt::message_t
{
	my_message( int x ) : m_x( x ) {}

	// Some data.
	int m_x;
};

// Another sample message.
struct my_another_message : public so_5::rt::message_t
{
	my_another_message( std::string s ) : m_s( std::move( s ) ) {}

	// Some data.
	std::string m_s;
};

// Sample message for the subscription demonstrtion.
class my_agent_t : public so_5::rt::agent_t
{
	public:
		my_agent_t( so_5::rt::environment_t & env );

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override;

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
		const so_5::rt::state_t st_first = so_make_state( "first" );
		const so_5::rt::state_t st_second = so_make_state( "second" );
};

my_agent_t::my_agent_t( so_5::rt::environment_t & env )
	: so_5::rt::agent_t( env )
{
}

void
my_agent_t::so_define_agent()
{
	std::cout << "so_define_agent()" << std::endl;

	st_first.event( &my_agent_t::change_state_event_handler );
	st_second.event( &my_agent_t::change_state_event_handler );

	so_default_state().event( &my_agent_t::change_state_event_handler );

	std::cout << "\tsubscribe my_event_handler in "
		<< st_first.query_name() << std::endl;

	st_first.event( &my_agent_t::my_event_handler );

	std::cout << "\tsubscribe my_another_event_handler in "
		<< st_first.query_name() << std::endl;

	st_first.event( &my_agent_t::my_another_event_handler );

	std::cout << "\tsubscribe my_event_handler in "
		<< st_second.query_name() << std::endl;

	st_second.event( &my_agent_t::my_event_handler );
}

void
my_agent_t::so_evt_start()
{
	std::cout << "so_evt_start()" << std::endl;

	std::cout << "\tsend sample messages sequence for state changes" << std::endl;
	// Send siries of messages.

	// Switch to first state and handle messages.
	so_5::send_to_agent< change_state_message >( *this, FIRST_STATE );
}

void
my_agent_t::change_state_event_handler(
	const change_state_message & message )
{
	std::cout << "change_state_event_handler()" << std::endl;

	if( DEFAULT_STATE == message.m_next_state )
	{
		this >>= so_default_state();

		std::cout << "\tswitched to default state and shutdown..." << std::endl;
		so_environment().stop();
	}
	else
	{
		if( FIRST_STATE == message.m_next_state )
		{
			this >>= st_first;

			std::cout << "\tswitched to " << so_current_state().query_name()
				<< std::endl;

			// Send serie of messages...
			so_5::send_to_agent< my_message >( *this, 42 );
			so_5::send_to_agent< my_another_message >( *this, "SObjectizer" );

			std::cout << "\tmessages sent" << std::endl;

			// Switch to second.
			so_5::send_to_agent< change_state_message >( *this, SECOND_STATE );
		}
		else if( SECOND_STATE == message.m_next_state )
		{
			this >>= st_second;

			std::cout << "\tswitched to " << so_current_state().query_name()
				<< std::endl;

			// Send serie of messages...
			so_5::send_to_agent< my_message >( *this, -42 );
			// Message should not be received.
			so_5::send_to_agent< my_another_message >( *this, "rezitcejbOS" );

			std::cout << "\tmessages sent" << std::endl;

			// Switch to default.
			so_5::send_to_agent< change_state_message >( *this, DEFAULT_STATE );
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
		<< "\tmessage.x = " << message.m_x << std::endl;
}

void
my_agent_t::my_another_event_handler(
	const my_another_message & message )
{
	std::cout << "my_another_event_handler()" << std::endl;

	std::cout
		<< "\tcurrent state is " << so_current_state().query_name() << std::endl
		<< "\tmessage.s = " << message.m_s << std::endl;
}


int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
				env.register_agent_as_coop( so_5::autoname,
						env.make_agent< my_agent_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


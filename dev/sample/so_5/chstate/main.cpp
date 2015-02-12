/*
 * A sample of the simpliest agent which has several states.
 * The agent uses different handlers for the same message.
 * At the beginning of its work agent initiates a periodic message.
 * Then agent handles this messages and switches from one state
 * to another.
 *
 * A work of the SObjectizer Environment is finished after the agent
 * switched to the final state.
 *
 * State switching is fixed by a state listener.
 */

#include <iostream>
#include <time.h>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Periodic message.
class msg_periodic : public so_5::rt::signal_t {};

// State listener for fixing state changes.
class state_monitor_t : public so_5::rt::agent_state_listener_t
{
	const std::string m_type_hint;

	public:
		state_monitor_t( const std::string & type_hint )
			:	m_type_hint( type_hint )
		{}

		virtual void
		changed(
			so_5::rt::agent_t &,
			const so_5::rt::state_t & state ) override
		{
			std::cout << m_type_hint << " agent changed state to "
				<< state.query_name()
				<< std::endl;
		}
};

// A sample agent class.
class a_state_swither_t : public so_5::rt::agent_t
{
		// Agent states.
		so_5::rt::state_t st_1 = so_make_state( "state_1" );
		so_5::rt::state_t st_2 = so_make_state( "state_2" );
		so_5::rt::state_t st_3 = so_make_state( "state_3" );
		so_5::rt::state_t st_shutdown = so_make_state( "shutdown" );

	public:
		a_state_swither_t( so_5::rt::environment_t & env )
			:	so_5::rt::agent_t( env )
		{}

		virtual ~a_state_swither_t()
		{}

		// Definition of the agent for SObjectizer.
		virtual void
		so_define_agent() override;

		// Reaction to start into SObjectizer.
		virtual void
		so_evt_start() override;

		// Message handler for the default state.
		void
		evt_handler_default();

		// Message handler for the state_1.
		void
		evt_handler_1();

		// Message handler for the state_2.
		void
		evt_handler_2();

		// Message handler for the state_3.
		void
		evt_handler_3();

		// Message handler for the shutdown_state.
		void
		evt_handler_shutdown();

	private:
		// Timer event id.
		// If we do not store it the periodic message will
		// be canceled automatically.
		so_5::timer_id_t m_timer_id;

		// Helper method for showing that event handler is called.
		void
		show_event_invocation( const char * event_name );
};

void
a_state_swither_t::so_define_agent()
{
	// Message subsription.
	so_default_state()
		.event< msg_periodic >( &a_state_swither_t::evt_handler_default );

	st_1.event< msg_periodic >( &a_state_swither_t::evt_handler_1 );

	st_2.event< msg_periodic >( &a_state_swither_t::evt_handler_2 );

	st_3.event< msg_periodic >( &a_state_swither_t::evt_handler_3 );

	st_shutdown.event< msg_periodic >( &a_state_swither_t::evt_handler_shutdown );
}

void
a_state_swither_t::so_evt_start()
{
	show_event_invocation( "so_evt_start()" );

	// Periodic message should be initiated.
	m_timer_id = so_environment().schedule_timer< msg_periodic >(
			so_direct_mbox(),
			std::chrono::seconds( 1 ),
			std::chrono::seconds( 1 ) );
}

void
a_state_swither_t::evt_handler_default()
{
	show_event_invocation( "evt_handler_default" );

	// Switching to the next state.
	so_change_state( st_1 );
}

void
a_state_swither_t::evt_handler_1()
{
	show_event_invocation( "evt_handler_1" );

	// Switching to the next state.
	so_change_state( st_2 );
}

void
a_state_swither_t::evt_handler_2()
{
	show_event_invocation( "evt_handler_2" );

	// Switching to the next state.
	so_change_state( st_3 );
}

void
a_state_swither_t::evt_handler_3()
{
	show_event_invocation( "evt_handler_3" );

	// Switching to the next state.
	so_change_state( st_shutdown );
}

void
a_state_swither_t::evt_handler_shutdown()
{
	show_event_invocation( "evt_handler_3" );

	// Switching to the default state.
	so_change_state( so_default_state() );

	// Finishing SObjectizer's work.
	std::cout << "Stop sobjectizer..." << std::endl;
	so_environment().stop();
}

void
a_state_swither_t::show_event_invocation( const char * event_name )
{
	time_t t = time( 0 );
	std::cout << asctime( localtime( &t ) )
		<< event_name << ", state: " << so_current_state().query_name()
		<< std::endl;
}

// A state listener.
state_monitor_t g_state_monitor( "nondestroyable_listener" );

// The SObjectizer Environment initialization.
void
init( so_5::rt::environment_t & env )
{
	std::unique_ptr< a_state_swither_t > ag( new a_state_swither_t( env ) );

	// Adding the state listener. Its lifetime is not controlled by the agent.
	ag->so_add_nondestroyable_listener( g_state_monitor );

	// Adding another state listener.
	// Its lifetime is controlled by the agent.
	ag->so_add_destroyable_listener(
		so_5::rt::agent_state_listener_unique_ptr_t(
			new state_monitor_t( "destroyable_listener" ) ) );

	// Creating and registering a cooperation.
	env.register_agent_as_coop( "coop", std::move(ag) );
}

int
main()
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


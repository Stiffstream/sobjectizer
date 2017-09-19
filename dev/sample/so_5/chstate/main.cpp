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

#if defined( _MSC_VER )
	#if defined( __clang__ )
		#pragma clang diagnostic ignored "-Wreserved-id-macro"
	#endif
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <time.h>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Periodic message.
class msg_periodic : public so_5::signal_t {};

// State listener for fixing state changes.
class state_monitor_t : public so_5::agent_state_listener_t
{
	const std::string m_type_hint;

	public:
		state_monitor_t( const std::string & type_hint )
			:	m_type_hint( type_hint )
		{}

		virtual void changed(
			so_5::agent_t &,
			const so_5::state_t & state ) override
		{
			std::cout << m_type_hint << " agent changed state to "
				<< state.query_name()
				<< std::endl;
		}
};

// A sample agent class.
class a_state_swither_t : public so_5::agent_t
{
		// Agent states.
		const state_t st_1{ this, "state_1" };
		const state_t st_2{ this, "state_2" };
		const state_t st_3{ this, "state_3" };
		const state_t st_shutdown{ this, "shutdown" };

	public:
		a_state_swither_t( context_t ctx )
			:	so_5::agent_t{ ctx }
		{}

		// Definition of the agent for SObjectizer.
		virtual void so_define_agent() override;

		// Reaction to start into SObjectizer.
		virtual void so_evt_start() override;

		// Message handler for the default state.
		void evt_handler_default();

		// Message handler for the state_1.
		void evt_handler_1();

		// Message handler for the state_2.
		void evt_handler_2();

		// Message handler for the state_3.
		void evt_handler_3();

		// Message handler for the shutdown_state.
		void evt_handler_shutdown();

	private:
		// Timer event id.
		// If we do not store it the periodic message will
		// be canceled automatically.
		so_5::timer_id_t m_timer_id;

		// Helper method for showing that event handler is called.
		void show_event_invocation( const char * event_name );
};

void a_state_swither_t::so_define_agent()
{
	// Message subsription.
	so_subscribe_self()
		.event< msg_periodic >( &a_state_swither_t::evt_handler_default );

	so_subscribe_self().in( st_1 )
		.event< msg_periodic >( &a_state_swither_t::evt_handler_1 );

	so_subscribe_self().in( st_2 )
		.event< msg_periodic >( &a_state_swither_t::evt_handler_2 );

	so_subscribe_self().in( st_3 )
		.event< msg_periodic >( &a_state_swither_t::evt_handler_3 );

	so_subscribe_self().in( st_shutdown )
		.event< msg_periodic >( &a_state_swither_t::evt_handler_shutdown );
}

void a_state_swither_t::so_evt_start()
{
	show_event_invocation( "so_evt_start()" );

	// Periodic message should be initiated.
	m_timer_id = so_5::send_periodic< msg_periodic >(
			*this,
			std::chrono::seconds( 1 ),
			std::chrono::seconds( 1 ) );
}

void a_state_swither_t::evt_handler_default()
{
	show_event_invocation( "evt_handler_default" );

	// Switching to the next state.
	so_change_state( st_1 );
}

void a_state_swither_t::evt_handler_1()
{
	show_event_invocation( "evt_handler_1" );

	// Switching to the next state.
	so_change_state( st_2 );
}

void a_state_swither_t::evt_handler_2()
{
	show_event_invocation( "evt_handler_2" );

	// Switching to the next state.
	so_change_state( st_3 );
}

void a_state_swither_t::evt_handler_3()
{
	show_event_invocation( "evt_handler_3" );

	// Switching to the next state.
	so_change_state( st_shutdown );
}

void a_state_swither_t::evt_handler_shutdown()
{
	show_event_invocation( "evt_handler_3" );

	// Switching to the default state.
	so_change_state( so_default_state() );

	// Finishing SObjectizer's work.
	std::cout << "Stop sobjectizer..." << std::endl;
	so_environment().stop();
}

void a_state_swither_t::show_event_invocation( const char * event_name )
{
	time_t t = time( nullptr );
	std::cout << asctime( localtime( &t ) )
		<< event_name << ", state: " << so_current_state().query_name()
		<< std::endl;
}

// A state listener.
state_monitor_t g_state_monitor( "nondestroyable_listener" );

// The SObjectizer Environment initialization.
void init( so_5::environment_t & env )
{
	auto ag = env.make_agent< a_state_swither_t >();

	// Adding the state listener. Its lifetime is not controlled by the agent.
	ag->so_add_nondestroyable_listener( g_state_monitor );

	// Adding another state listener.
	// Its lifetime is controlled by the agent.
	ag->so_add_destroyable_listener(
		so_5::agent_state_listener_unique_ptr_t(
			new state_monitor_t( "destroyable_listener" ) ) );

	// Creating and registering a cooperation.
	env.register_agent_as_coop( "coop", std::move(ag) );
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


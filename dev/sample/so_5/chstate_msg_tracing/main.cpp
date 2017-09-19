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
 * A message delivery tracing is enabled. Trace is going to std::cout.
 */

#include <iostream>
#include <time.h>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A sample agent class.
class a_state_swither_t : public so_5::agent_t
{
	// Signal for changing agent state.
	struct change_state_signal : public so_5::signal_t {};

	// Demo message for showing different handlers in different states.
	struct greeting_message
	{
		const std::string m_greeting;
	};

	// Agent states.
	const state_t st_1{ this, "state_1" };
	const state_t st_2{ this, "state_2" };
	const state_t st_3{ this, "state_3" };
	const state_t st_shutdown{ this, "shutdown" };

public:
	a_state_swither_t( context_t ctx ) : so_5::agent_t{ ctx }
	{}

	virtual ~a_state_swither_t() override
	{}

	// Definition of the agent for SObjectizer.
	virtual void
	so_define_agent() override
	{
		// Actions for the default state.
		so_default_state()
			.event< change_state_signal >( [=] { this >>= st_1; } )
			.event( [=]( const greeting_message & msg ) {
					std::cout << "*** 0) greeting: " << msg.m_greeting
							<< ", ptr: " << &msg << std::endl;
				} );

		// Actions for other states.

		// st_1: switch to st_2 only, greeting_message is ignored.
		st_1
			.event< change_state_signal >( [=] { this >>= st_2; } );

		// st_2: switch to st_3, greeting_message is handled.
		st_2
			.event< change_state_signal >( [=] { this >>= st_3; } )
			.event( [=]( const greeting_message & msg ) {
					std::cout << "*** 2) greeting: " << msg.m_greeting
							<< ", ptr: " << &msg << std::endl;
				} );

		// st_3: switch to st_shutdown only, greeting_message is ignored.
		st_3
			.event< change_state_signal >( [=] { this >>= st_shutdown; } );

		// st_shutdown: handle greeting_message, shutdown environment.
		st_shutdown
			.event< change_state_signal >( [=] {
					so_deregister_agent_coop_normally(); } )
			.event( [=]( const greeting_message & msg ) {
					std::cout << "*** F) greeting: " << msg.m_greeting
							<< ", ptr: " << &msg << std::endl;
				} );
	}

	// Reaction to start inside SObjectizer.
	virtual void so_evt_start() override
	{
		m_greeting_timer_id = so_5::send_periodic< greeting_message >(
				*this,
				std::chrono::milliseconds{ 50 },
				std::chrono::milliseconds{ 100 },
				"Hello, World!" );
		m_change_timer_id = so_5::send_periodic< change_state_signal >(
				*this,
				std::chrono::milliseconds{ 80 },
				std::chrono::milliseconds{ 100 } );
	}

private:
	// Timer event id.
	// If we do not store it the periodic message will
	// be canceled automatically.
	so_5::timer_id_t m_greeting_timer_id;
	so_5::timer_id_t m_change_timer_id;
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env ) {
				env.introduce_coop( []( so_5::coop_t & coop ) {
					coop.make_agent< a_state_swither_t >();
				} );
			},
			[]( so_5::environment_params_t & params ) {
				// Turn message delivery tracing on.
				params.message_delivery_tracer(
						so_5::msg_tracing::std_cout_tracer() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


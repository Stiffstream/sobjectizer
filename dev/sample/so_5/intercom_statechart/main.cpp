/*
 * An example of implementation of hierarchical state machine by using
 * agent's states.
 */

#include <iostream>
#include <string>
#include <cctype>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Messages to be used for interaction with intercom agents.
struct key_cancel : public so_5::signal_t {};
struct key_bell : public so_5::signal_t {};
struct key_grid : public so_5::signal_t {};

struct key_digit
{
	char m_value;
};

// Private messages for intercom implementation.
namespace intercom_messages
{

struct activated : public so_5::signal_t {};
struct deactivate : public so_5::signal_t {};

struct display_text
{
	std::string m_what;
};

// Helper function for sending display_text message.
inline void show_on_display(
	const so_5::mbox_t & intercom_mbox,
	std::string what )
{
	so_5::send< display_text >( intercom_mbox, std::move(what) );
}

// Helper function for clearing display.
inline void clear_display(
	const so_5::mbox_t & intercom_mbox )
{
	so_5::send< display_text >( intercom_mbox, "" );
}

} // namespace intercom_messages

// An agent for controlling inactivity time.
//
// Listen the same user-activity messages as controller agent and reschedules
// delayed message after each of them. This delayed message tells the
// controller to change state from active to inactive.
//
// Please note: this agent switches from inactive to active state by the
// signal from controller agent.
class inactivity_watcher final : public so_5::agent_t
{
	state_t inactive{ this, "inactive" };
	state_t active{ this, "active" };

	const std::chrono::seconds inactivity_time{ 10 };

public :

	inactivity_watcher(
		context_t ctx,
		so_5::mbox_t intercom_mbox )
		:	so_5::agent_t{ ctx }
		,	m_intercom_mbox{ std::move(intercom_mbox) }
	{
		inactive
			.on_enter( [this] { m_timer.release(); } )
			.just_switch_to< intercom_messages::activated >(
					m_intercom_mbox, active );
		
		active
			.on_enter( [this] { reschedule_timer(); } )
			.event< key_cancel >( m_intercom_mbox, [this] { reschedule_timer(); } )
			.event< key_bell >( m_intercom_mbox, [this] { reschedule_timer(); } )
			.event< key_grid >( m_intercom_mbox, [this] { reschedule_timer(); } )
			.event(
					m_intercom_mbox,
					[this]( const key_digit & ) {
						reschedule_timer();
					} )
			.just_switch_to< intercom_messages::deactivate >(
					m_intercom_mbox, inactive );

		this >>= inactive;
	}

private :
	const so_5::mbox_t m_intercom_mbox;
	so_5::timer_id_t m_timer;

	void reschedule_timer()
	{
		m_timer = so_5::send_periodic< intercom_messages::deactivate >(
				so_environment(),
				m_intercom_mbox,
				inactivity_time,
				std::chrono::seconds::zero() );
	}
};

// An agent for imitation of keyboard's light equipment.
//
// Switching from off to on states and back is done by listening
// activated and deactivate signals.
class keyboard_lights final : public so_5::agent_t
{
	state_t off{ this, "off" };
	state_t on{ this, "on" };

public :
	keyboard_lights(
		context_t ctx,
		const so_5::mbox_t & intercom_mbox )
		:	so_5::agent_t{ ctx }
	{
		off
			.on_enter( []{ std::cout << "keyboard_lights OFF" << std::endl; } )
			.just_switch_to< intercom_messages::activated >(
					intercom_mbox, on );

		on
			.on_enter( []{ std::cout << "keyboard_lights ON" << std::endl; } )
			.just_switch_to< intercom_messages::deactivate >(
					intercom_mbox, off );

		this >>= off;
	}
};

// An agent for imitation of intercom's display.
//
// Switching from off to on states and back is done by listening
// activated and deactivate signals.
class display final : public so_5::agent_t
{
	state_t off{ this, "off" };
	state_t on{ this, "on" };

public :
	display(
		context_t ctx,
		const so_5::mbox_t & intercom_mbox )
		:	so_5::agent_t{ ctx }
	{
		off
			.on_enter( []{ std::cout << "display OFF" << std::endl; } )
			.just_switch_to< intercom_messages::activated >(
					intercom_mbox, on );

		on
			.on_enter( []{ std::cout << "display ON" << std::endl; } )
			.event( intercom_mbox,
					[]( const intercom_messages::display_text & msg ) {
						std::cout << "display: '" << msg.m_what << "'" << std::endl;
					} )
			.just_switch_to< intercom_messages::deactivate >(
					intercom_mbox, off );

		this >>= off;
	}
};

// An agent for imitation of call to an apartment.
//
// Just changes a message on display for every 1.5s.
class ringer final : public so_5::agent_t
{
	state_t
		off{ this, "off" },
		on{ this, "on" },
			ringing{ initial_substate_of{ on }, "ringing" },
			sleeping{ substate_of{ on }, "sleeping" };

	struct timer : public so_5::signal_t {};

public :
	struct dial_to { std::string m_number; };
	struct stop_dialing : public so_5::signal_t {};

	ringer(
		context_t ctx,
		so_5::mbox_t intercom_mbox )
		:	so_5::agent_t{ ctx }
		,	m_intercom_mbox{ std::move(intercom_mbox) }
	{
		this >>= off;

		off
			.on_enter( [this]{ m_timer.release(); } )
			.event( m_intercom_mbox, [this]( const dial_to & msg ) {
					m_number = msg.m_number;
					this >>= on;
				} );

		on
			.on_enter( [this] {
					m_timer = so_5::send_periodic< timer >(
							*this,
							std::chrono::milliseconds::zero(),
							std::chrono::milliseconds{ 1500 } );
				} )
			.just_switch_to< stop_dialing >( m_intercom_mbox, off );

		ringing
			.on_enter( [this]{ 
					intercom_messages::show_on_display(
							m_intercom_mbox, "RING" );
				} )
			.just_switch_to< timer >( sleeping );

		sleeping
			.on_enter( [this]{
					intercom_messages::show_on_display(
							m_intercom_mbox, m_number );
				} )
			.just_switch_to< timer >( ringing );
	}

private :
	const so_5::mbox_t m_intercom_mbox;

	so_5::timer_id_t m_timer;

	std::string m_number;
};

// Main agent of intercom example.
// Imitates intercom's controller.
//
// Receives a messages from user and perform various actions.
//
class controller final : public so_5::agent_t
{
	// States of agent's statechart.
	state_t
		inactive{ this, "inactive" },
		active{ this, "active" },

			wait_activity{
					initial_substate_of{ active }, "wait_activity" },
			number_selection{ substate_of{ active }, "number_selection" },

			dialling{ substate_of{ active }, "dialling" },

				dial_apartment{
						initial_substate_of{ dialling }, "dial_apartment" },
				no_answer{
						substate_of{ dialling }, "no_answer" },

			special_code_selection{
					substate_of{ active }, "special_code_selection" },

				special_code_selection_0{
						initial_substate_of{ special_code_selection },
						"special_code_selection_0" },

				user_code_selection{
						substate_of{ special_code_selection },
						"user_code_selection" },
					user_code_apartment_number{
							initial_substate_of{ user_code_selection },
							"apartment_number" },
					user_code_secret{
							substate_of{ user_code_selection },
							"secret_code" },

				service_code_selection{
						substate_of{ special_code_selection },
						"service_code" },
				door_unlocked{
						substate_of{ special_code_selection },
						"door_unlocked" }
	;

	struct apartment_info
	{
		std::string m_number;
		std::string m_secret_key;

		apartment_info( std::string n, std::string k )
			:	m_number( std::move(n) ), m_secret_key( std::move(k) )
		{}
	};

public :
	controller(
		context_t ctx,
		so_5::mbox_t intercom_mbox )
		:	so_5::agent_t{ ctx }
		,	m_intercom_mbox{ std::move(intercom_mbox) }
		,	m_apartments( make_apartment_info() )
		,	m_actual_service_code( "12345" )
	{
		// Setting up statechart.
		inactive
			.transfer_to_state< key_digit >( m_intercom_mbox, active )
			.transfer_to_state< key_grid >( m_intercom_mbox, active )
			.transfer_to_state< key_bell >( m_intercom_mbox, active )
			.transfer_to_state< key_cancel >( m_intercom_mbox, active );

		active
			.on_enter( &controller::active_on_enter )
			.event( m_intercom_mbox, &controller::active_on_grid )
			.event( m_intercom_mbox, &controller::active_on_cancel )
			.just_switch_to< intercom_messages::deactivate >(
					m_intercom_mbox, inactive );

		wait_activity
			.transfer_to_state< key_digit >( m_intercom_mbox, number_selection );

		number_selection
			.on_enter( &controller::apartment_number_on_enter )
			.event( m_intercom_mbox, &controller::apartment_number_on_digit )
			.event( m_intercom_mbox, &controller::apartment_number_on_bell )
			.suppress< key_grid >( m_intercom_mbox );

		dialling
			.suppress< key_grid >( m_intercom_mbox )
			.suppress< key_bell >( m_intercom_mbox )
			.suppress< key_digit >( m_intercom_mbox );

		dial_apartment
			.time_limit( std::chrono::seconds{ 8 }, no_answer )
			.on_enter( &controller::dial_apartment_on_enter )
			.on_exit( &controller::dial_apartment_on_exit );

		no_answer
			.time_limit( std::chrono::milliseconds{ 1500 }, wait_activity )
			.on_enter( &controller::no_answer_on_enter )
			.on_exit( &controller::no_answer_on_exit )
			.suppress< key_cancel >( m_intercom_mbox );

		special_code_selection_0
			.transfer_to_state< key_digit >( m_intercom_mbox, user_code_selection )
			.just_switch_to< key_grid >( m_intercom_mbox, service_code_selection );

		user_code_apartment_number
			.on_enter( &controller::user_code_apartment_number_on_enter )
			.event( m_intercom_mbox, &controller::apartment_number_on_digit )
			.just_switch_to< key_grid >( m_intercom_mbox, user_code_secret );

		user_code_secret
			.on_enter( &controller::user_code_secret_on_enter )
			.event( m_intercom_mbox, &controller::user_code_secret_on_digit )
			.event( m_intercom_mbox, &controller::user_code_secret_on_bell );

		service_code_selection
			.on_enter( &controller::service_code_on_enter )
			.event( m_intercom_mbox, &controller::service_code_on_digit )
			.event( m_intercom_mbox, &controller::service_code_on_grid );

		door_unlocked
			.time_limit( std::chrono::seconds{ 5 }, wait_activity )
			.on_enter( &controller::door_unlocked_on_enter )
			.on_exit( &controller::door_unlocked_on_exit )
			.suppress< key_grid >( m_intercom_mbox )
			.suppress< key_bell >( m_intercom_mbox )
			.suppress< key_digit >( m_intercom_mbox )
			.suppress< key_cancel >( m_intercom_mbox );
	}

	virtual void so_evt_start() override
	{
		// Agent starts in the default state. Switch to the appropriate one.
		this >>= inactive;
	}

private :
	// Limitations for various pieces of user input.
	static const std::size_t max_apartment_number_size = 3u;
	static const std::size_t max_secret_code_size = 4u;
	static const std::size_t service_code_size = 5u;

	// Common mbox for all intercom's parts.
	const so_5::mbox_t m_intercom_mbox;
	// Information about apartments and their secret codes.
	const std::vector< apartment_info > m_apartments;

	// Accumutator for apartment number.
	std::string m_apartment_number;

	// Accumutator for user's secret code.
	std::string m_user_secret_code;

	// Accumutator for service code.
	std::string m_service_code;
	// Service code value for this intercom instance.
	const std::string m_actual_service_code;

	// Helper method for generation of apartments' information.
	static std::vector< apartment_info > make_apartment_info()
	{
		std::vector< apartment_info > result;
		result.reserve( 10 );

		result.emplace_back( "101", "1011" );
		result.emplace_back( "102", "1022" );
		result.emplace_back( "103", "1033" );
		result.emplace_back( "104", "1044" );
		result.emplace_back( "105", "1055" );
		result.emplace_back( "106", "1066" );
		result.emplace_back( "107", "1077" );
		result.emplace_back( "108", "1088" );
		result.emplace_back( "109", "1099" );
		result.emplace_back( "110", "1100" );

		return result;
	}

	void active_on_enter()
	{
		// Special signal must be send on enter to active state.
		// This signal will activate other parts of intercom.
		so_5::send< intercom_messages::activated >( m_intercom_mbox );
	}

	void active_on_cancel( mhood_t< key_cancel > )
	{
		this >>= wait_activity;
	}

	void active_on_grid( mhood_t< key_grid > )
	{
		this >>= special_code_selection;
	}

	void apartment_number_on_enter()
	{
		// Old value of accumulator must be dropped to allow to accumulate new
		// one.
		m_apartment_number.clear();
	}

	void apartment_number_on_digit( const key_digit & msg )
	{
		if( m_apartment_number.size() < max_apartment_number_size )
			m_apartment_number += msg.m_value;

		intercom_messages::show_on_display(
				m_intercom_mbox, m_apartment_number );
	}

	void apartment_number_on_bell( mhood_t< key_bell > )
	{
		auto apartment = std::find_if( begin(m_apartments), end(m_apartments),
				[this]( const apartment_info & info ) {
					return info.m_number == m_apartment_number;
				} );

		if( apartment != end(m_apartments) )
			this >>= dialling;
		else
		{
			intercom_messages::show_on_display( m_intercom_mbox, "Err" );
			this >>= wait_activity;
		}
	}

	void dial_apartment_on_enter()
	{
		so_5::send< ringer::dial_to >( m_intercom_mbox, m_apartment_number );
	}

	void dial_apartment_on_exit()
	{
		so_5::send< ringer::stop_dialing >( m_intercom_mbox );
	}

	void no_answer_on_enter()
	{
		intercom_messages::show_on_display( m_intercom_mbox, "No Answer" );
	}

	void no_answer_on_exit()
	{
		intercom_messages::clear_display( m_intercom_mbox );
	}

	void user_code_apartment_number_on_enter()
	{
		// Old value of accumulator must be dropped to allow to accumulate new
		// one.
		m_apartment_number.clear();
	}

	void user_code_secret_on_enter()
	{
		// Old value of accumulator must be dropped to allow to accumulate new
		// one.
		m_user_secret_code.clear();
		intercom_messages::clear_display( m_intercom_mbox );
	}

	void user_code_secret_on_digit( const key_digit & msg )
	{
		if( m_user_secret_code.size() < max_secret_code_size )
			m_user_secret_code += msg.m_value;

		intercom_messages::show_on_display(
				m_intercom_mbox, 
				std::string( m_user_secret_code.size(), '*' ) );
	}

	void user_code_secret_on_bell( mhood_t< key_bell > )
	{
		auto apartment = std::find_if( begin(m_apartments), end(m_apartments),
				[this]( const apartment_info & info ) {
					return info.m_number == m_apartment_number;
				} );

		if( apartment != end(m_apartments) &&
				m_user_secret_code == apartment->m_secret_key )
			this >>= door_unlocked;
		else
		{
			intercom_messages::show_on_display( m_intercom_mbox, "Err" );
			this >>= wait_activity;
		}
	}

	void door_unlocked_on_enter()
	{
		intercom_messages::show_on_display( m_intercom_mbox, "unlocked" );
	}

	void door_unlocked_on_exit()
	{
		intercom_messages::clear_display( m_intercom_mbox );
	}

	void service_code_on_enter()
	{
		// Old value of accumulator must be dropped to allow to accumulate new
		// one.
		m_service_code.clear();
	}

	void service_code_on_digit( const key_digit & msg )
	{
		if( m_service_code.size() < service_code_size )
			m_service_code += msg.m_value;

		intercom_messages::show_on_display(
				m_intercom_mbox, 
				std::string( m_service_code.size(), '#' ) );
	}

	void service_code_on_grid( mhood_t< key_grid > )
	{
		if( !m_service_code.empty() )
		{
			if( m_service_code == m_actual_service_code )
				this >>= door_unlocked;
			else
			{
				intercom_messages::show_on_display( m_intercom_mbox, "Err" );
				this >>= wait_activity;
			}
		}
	}
};

// Helper for creation of coop with intercom's agents.
so_5::mbox_t create_intercom( so_5::environment_t & env )
{
	so_5::mbox_t intercom_mbox;
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		intercom_mbox = env.create_mbox();

		coop.make_agent< controller >( intercom_mbox );
		coop.make_agent< inactivity_watcher >( intercom_mbox );
		coop.make_agent< keyboard_lights >( intercom_mbox );
		coop.make_agent< display >( intercom_mbox );
		coop.make_agent< ringer >( intercom_mbox );
	} );

	return intercom_mbox;
}

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj/*{
		[]( so_5::environment_t & ) {},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer( so_5::msg_tracing::std_clog_tracer() );
		} }*/;

	auto intercom = create_intercom( sobj.environment() );

	while( true )
	{
		std::cout << "enter digit or 'c' or 'b' or '#' (or 'exit' to stop): "
			<< std::flush;

		std::string choice;
		std::cin >> choice;

		if( "c" == choice ) so_5::send< key_cancel >( intercom );
		else if( "b" == choice ) so_5::send< key_bell >( intercom );
		else if( "#" == choice ) so_5::send< key_grid >( intercom );
		else if( "exit" == choice ) break;
		else if( 1 == choice.size() && std::isdigit( choice[ 0 ] ) )
			so_5::send< key_digit >( intercom, choice[ 0 ] );
	}

	// SObjectizer will be stopped automatically.
}

int main()
{
	try
	{
		demo();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 0;
}


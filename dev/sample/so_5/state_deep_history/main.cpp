/*
 * An example of usage of agent's state with deep history.
 */

#include <iostream>
#include <string>
#include <cctype>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Messages to be used for interaction with console agent.
struct key_cancel : public so_5::signal_t {};
struct key_bell : public so_5::signal_t {};
struct key_grid : public so_5::signal_t {};

struct key_digit
{
	char m_value;
};

// Main agent.
class console final : public so_5::agent_t
{
	// States of agent's statechart.
	state_t
		dialog{ this, "dialog", deep_history },

			wait_activity{
					initial_substate_of{ dialog }, "wait_activity" },
			number_selection{ substate_of{ dialog }, "number_selection" },

			special_code_selection{
					substate_of{ dialog }, "special_code_selection" },

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


			operation_completed{ substate_of{ dialog }, "op_completed" },

		show_error{ this, "error" }
	;

public :
	console( context_t ctx ) :	so_5::agent_t{ ctx }
	{
		// Setting up statechart.
		dialog
			.event( &console::dialog_on_grid )
			.event( &console::dialog_on_cancel );

		wait_activity
			.on_enter( &console::wait_activity_on_enter )
			.transfer_to_state< key_digit >( number_selection );

		number_selection
			.on_enter( &console::apartment_number_on_enter )
			.event( &console::apartment_number_on_digit )
			.event( &console::apartment_number_on_bell )
			.event( &console::apartment_number_on_grid );

		special_code_selection_0
			.transfer_to_state< key_digit >( user_code_selection )
			.just_switch_to< key_grid >( service_code_selection );

		user_code_apartment_number
			.on_enter( &console::user_code_apartment_number_on_enter )
			.event( &console::apartment_number_on_digit )
			.event( &console::user_code_apartment_number_on_bell )
			.event( &console::user_code_apartment_number_on_grid );

		user_code_secret
			.on_enter( &console::user_code_secret_on_enter )
			.event( &console::user_code_secret_on_digit )
			.event( &console::user_code_secret_on_bell )
			.event( &console::user_code_secret_on_grid );

		service_code_selection
			.on_enter( &console::service_code_on_enter )
			.event( &console::service_code_on_digit )
			.event( &console::service_code_on_bell )
			.event( &console::service_code_on_grid );

		operation_completed
			.on_enter( &console::op_completed_on_enter )
			.time_limit( std::chrono::seconds{3}, wait_activity );

		show_error
			.on_enter( &console::show_error_on_enter )
			.on_exit( &console::show_error_on_exit )
			.time_limit( std::chrono::seconds{2}, dialog );
	}

	virtual void so_evt_start() override
	{
		// Agent starts in the default state. Switch to the appropriate one.
		this >>= dialog;
	}

private :
	// Helper class for imitiation of console's display.
	class display
	{
	public :
		void show( const std::string & what )
		{
			std::cout << "display, msg: '" << what << "'" << std::endl;
		}

		void show_error( const std::string & what )
		{
			std::cout << "display, ERR: '" << what << "'" << std::endl;
		}

		void clear()
		{
			std::cout << "display cleared" << std::endl;
		}
	};

	// Limitations for various pieces of user input.
	static const std::size_t apartment_number_size = 3u;
	static const std::size_t secret_code_size = 4u;
	static const std::size_t service_code_size = 5u;

	// Accumutator for apartment number.
	std::string m_apartment_number;

	// Accumutator for user's secret code.
	std::string m_user_secret_code;

	// Accumutator for service code.
	std::string m_service_code;

	// Message to be displayed in show_error state.
	std::string m_error_message;
	// Message to be displayed in operation_completed state.
	std::string m_op_result_message;

	// Display imitator.
	display m_display;

	void dialog_on_cancel( mhood_t< key_cancel > )
	{
		// key_cancel always leads to wait_activity state.
		// We must switch to wait_activity but not to dialog state
		// because dialog has history and switch to dialog state will
		// lead to restoration of last active dialog's substate.
		this >>= wait_activity;
	}

	void dialog_on_grid( mhood_t< key_grid > )
	{
		this >>= special_code_selection;
	}

	void wait_activity_on_enter()
	{
		// We can enter in that state only on the very first enter into dialog
		// state (because wait_activity is the initial substate). Or after
		// completion of some operator. Or after cancelation of any operation.
		// Because of that all accumulators can be cleared here.
		m_apartment_number.clear();
		m_user_secret_code.clear();
		m_service_code.clear();

		m_display.clear();
	}

	void apartment_number_on_enter()
	{
		// Show the current value of accumulator on reentry into this state.
		if( !m_apartment_number.empty() )
			m_display.show( m_apartment_number );
	}

	void apartment_number_on_digit( const key_digit & msg )
	{
		if( m_apartment_number.size() < apartment_number_size )
		{
			m_apartment_number += msg.m_value;
			m_display.show( m_apartment_number );
		}
		else
			initiate_error( "apartment number must be " +
					std::to_string( apartment_number_size ) + " digits long" );
	}

	void apartment_number_on_bell( mhood_t< key_bell > )
	{
		if( m_apartment_number.size() == apartment_number_size )
		{
			complete_operation( "dial to apartment #" + m_apartment_number );
		}
		else
			initiate_error( "apartment number must be " +
					std::to_string( apartment_number_size ) + " digits long" );
	}

	void apartment_number_on_grid( mhood_t< key_grid > )
	{
		initiate_error( "enter apartment number, then 'b'" );
	}

	void user_code_apartment_number_on_enter()
	{
		// Show the current value of accumulator on reentry into this state.
		if( !m_apartment_number.empty() )
			m_display.show( m_apartment_number );
	}

	void user_code_apartment_number_on_bell( mhood_t< key_bell > )
	{
		initiate_error( "enter apartment number, then '#', then secret code, "
				"then 'b'" );
	}

	void user_code_apartment_number_on_grid( mhood_t< key_grid > )
	{
		if( m_apartment_number.size() == apartment_number_size )
		{
			this >>= user_code_secret;
		}
		else
			initiate_error( "apartment number must be " +
					std::to_string( apartment_number_size ) + " digits long" );
	}

	void user_code_secret_on_enter()
	{
		// Show the current value of accumulator on reentry into this state.
		if( !m_user_secret_code.empty() )
			m_display.show( std::string( m_user_secret_code.size(), '*' ) );
	}

	void user_code_secret_on_digit( const key_digit & msg )
	{
		if( m_user_secret_code.size() < secret_code_size )
		{
			m_user_secret_code += msg.m_value;
			m_display.show( std::string( m_user_secret_code.size(), '*' ) );
		}
		else
			initiate_error( "secret code must be " +
					std::to_string( secret_code_size ) + " digits long" );
	}

	void user_code_secret_on_bell( mhood_t< key_bell > )
	{
		if( m_user_secret_code.size() == secret_code_size )
		{
			complete_operation( "open the door via user secret code: " +
					m_apartment_number + "#" + m_user_secret_code );
		}
		else
			initiate_error( "secret code be " +
					std::to_string( secret_code_size ) + " digits long" );
	}

	void user_code_secret_on_grid( mhood_t< key_grid > )
	{
		initiate_error( "enter user secret code, then 'b'" );
	}

	void service_code_on_enter()
	{
		// Show the current value of accumulator on reentry into this state.
		if( !m_service_code.empty() )
			m_display.show( std::string( m_service_code.size(), '#' ) );
	}

	void service_code_on_digit( const key_digit & msg )
	{
		if( m_service_code.size() < service_code_size )
		{
			m_service_code += msg.m_value;
			m_display.show( std::string( m_service_code.size(), '#' ) );
		}
		else
			initiate_error( "service code be " +
					std::to_string( service_code_size ) + " digits long" );
	}

	void service_code_on_bell( mhood_t< key_bell > )
	{
		initiate_error( "enter service code, then '#'" );
	}

	void service_code_on_grid( mhood_t< key_grid > )
	{
		if( m_service_code.size() == service_code_size )
		{
			complete_operation( "open the door via service code: " + m_service_code );
		}
		else
			initiate_error( "service code must be " +
					std::to_string( service_code_size ) + " digits long" );
	}

	void op_completed_on_enter()
	{
		m_display.show( m_op_result_message );
	}

	void show_error_on_enter()
	{
		m_display.show_error( m_error_message );
	}

	void show_error_on_exit()
	{
		m_display.clear();
	}

	void initiate_error( const std::string & what )
	{
		m_error_message = what;
		this >>= show_error;
	}

	void complete_operation( const std::string & what )
	{
		m_op_result_message = what;
		this >>= operation_completed;
	}
};

// Helper for creation of coop with console agent.
so_5::mbox_t create_console( so_5::environment_t & env )
{
	so_5::mbox_t console_mbox;
	env.introduce_coop( [&]( so_5::coop_t & coop ) {
		console_mbox = coop.make_agent< console >()->so_direct_mbox();
	} );

	return console_mbox;
}

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj/*{
		[]( so_5::environment_t & ) {},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer( so_5::msg_tracing::std_cerr_tracer() );
		} }*/;

	auto console_mbox = create_console( sobj.environment() );

	while( true )
	{
		std::cout << "enter digit or 'c' or 'b' or '#' (or 'exit' to stop): "
			<< std::flush;

		std::string choice;
		std::cin >> choice;

		if( "c" == choice ) so_5::send< key_cancel >( console_mbox );
		else if( "b" == choice ) so_5::send< key_bell >( console_mbox );
		else if( "#" == choice ) so_5::send< key_grid >( console_mbox );
		else if( "exit" == choice ) break;
		else if( 1 == choice.size() && std::isdigit( choice[ 0 ] ) )
			so_5::send< key_digit >( console_mbox, choice[ 0 ] );
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


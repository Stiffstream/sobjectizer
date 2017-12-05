/*
 * A very simple demo of wrapped_env_t.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A simple agent which will be used in demo.
class demo_agent : public so_5::agent_t
{
public :
	// Ping signal to which agent must respond.
	struct ping : public so_5::signal_t {};

	demo_agent( context_t ctx ) :	so_5::agent_t{ ctx } {}

	virtual void so_define_agent() override
	{
		// Ping signals will be sent to named mbox.
		// The name of this mbox is the same as the name of agent's coop.
		so_subscribe( so_environment().create_mbox( so_coop_name() ) )
			.event< ping >( &demo_agent::evt_ping );
	}

	// Reaction to start of work inside SO Environment.
	virtual void so_evt_start() override
	{
		std::cout << so_coop_name() << " started" << std::endl;
	}

	// Reaction to end of work inside SO Environment.
	virtual void so_evt_finish() override
	{
		std::cout << so_coop_name() << " finished" << std::endl;
	}

private :
	// Reaction to ping signal.
	std::string evt_ping()
	{
		return "pong{" + so_coop_name() + "}";
	}
};

// Helper wrapper for doing some action and handling exceptions.
template< typename Lambda >
void do_action(
	const std::string & action_name,
	Lambda && lambda )
{
	try
	{
		lambda();
	}
	catch( const std::exception & x ) 
	{
		std::cerr << "Error during '" << action_name << "': " << x.what() << std::endl;
	}
}

// Main demo loop.
void demo()
{
	// Helper to ask coop's name from user.
	auto ask_coop_name = [] {
		std::cout << "Coop name: " << std::flush;
		std::string name;
		std::cin >> name;
		return name;
	};

	// SO Environment in a special wrapper object.
	// Environment will be started automatically.
	so_5::wrapped_env_t env;

	// Main loop. The demo will be finished when user enter 'exit' or 'quit' command.
	while( true )
	{
		std::cout << "Enter command (reg,dereg,ping,exit): " << std::flush;
		std::string choice;
		std::cin >> choice;

		if( "reg" == choice )
		{
			// Register new coop with user supplied name.
			const auto name = ask_coop_name();
			do_action( "registering coop '" + name + "'",
				[&] {
					env.environment().introduce_coop( name, []( so_5::coop_t & coop ) {
						coop.make_agent< demo_agent >();
					} );
				} );
		}
		else if( "dereg" == choice )
		{
			// Deregister coop with user supplied name.
			const auto name = ask_coop_name();
			do_action( "deregistering coop '" + name + "'",
				[&] {
					env.environment().deregister_coop( name, so_5::dereg_reason::normal );
				} );
		}
		else if( "ping" == choice )
		{
			// Try to ping coop with user supplied name.
			const auto name = ask_coop_name();
			do_action( "pinging '" + name + "'",
				[&] {
					// A synchronous request will be used for ping.
					// Ping signal will be sent to mbox with the name of coop.
					auto reply = so_5::request_value< std::string, demo_agent::ping >(
							// Mbox will be created if necessary.
							// If such mbox is already exists it will be reused.
							env.environment().create_mbox( name ),
							// Infinite wait for response.
							so_5::infinite_wait );

					std::cout << "ping reply: " << reply << std::endl;
				} );
		}
		else if( "exit" == choice || "quit" == choice )
			break;
	}

	// SO Environment will be automatically stopped.
}

int main()
{
	try
	{
		demo();
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
		return 2;
	}

	return 0;
}


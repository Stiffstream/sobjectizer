/*
 * Testing a cooperation registration/deregistration.
 *
 * Several cooperations are registered. Then part of them are deregistered.
 */

#include <iostream>
#include <map>
#include <exception>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

// A dummy agent to be placed into test cooperations.
class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		test_agent_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~test_agent_t() {}
};

void
reg_coop(
	const std::string & coop_name,
	so_5::environment_t & env )
{
	so_5::coop_unique_ptr_t coop = env.create_coop( coop_name );

	coop->add_agent( new test_agent_t( env ) );
	coop->add_agent( new test_agent_t( env ) );

	env.register_coop( std::move( coop ) );
}

void
init( so_5::environment_t & env )
{
	reg_coop( "test_coop_1", env );
	reg_coop( "test_coop_2", env );
	reg_coop( "test_coop_3", env );
	reg_coop( "test_coop_4", env );
	reg_coop( "test_coop_5", env );
	reg_coop( "test_coop_6", env );

	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

	env.deregister_coop( "test_coop_1", so_5::dereg_reason::normal );

	env.deregister_coop( "test_coop_6", so_5::dereg_reason::normal );

	env.deregister_coop( "test_coop_3", so_5::dereg_reason::normal );

	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

	env.stop();
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
		std::cerr << "error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


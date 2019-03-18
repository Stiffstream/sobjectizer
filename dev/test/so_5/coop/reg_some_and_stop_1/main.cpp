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

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

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

so_5::coop_handle_t
reg_coop(
	so_5::environment_t & env )
{
	auto coop = env.make_coop();

	coop->make_agent< test_agent_t >();
	coop->make_agent< test_agent_t >();

	return env.register_coop( std::move( coop ) );
}

void
init( so_5::environment_t & env )
{
	auto coop_1 = reg_coop( env );
	reg_coop( env );
	auto coop_3 = reg_coop( env );
	reg_coop( env );
	reg_coop( env );
	auto coop_6 = reg_coop( env );

	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

	env.deregister_coop( coop_1, so_5::dereg_reason::normal );

	env.deregister_coop( coop_6, so_5::dereg_reason::normal );

	env.deregister_coop( coop_3, so_5::dereg_reason::normal );

	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

	env.stop();
}


int
main()
{
	run_with_time_limit( [] {
			so_5::launch( init );
		},
		10 );

	return 0;
}


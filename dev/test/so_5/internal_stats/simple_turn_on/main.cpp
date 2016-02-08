/*
 * A simple test for turning run-time monitoring on.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

void
init( so_5::environment_t & env )
{
	env.stats_controller().turn_on();

	auto coop = env.create_coop( so_5::autoname );
	coop->define_agent()
		.on_start( [&env] { env.stop(); } );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple stats_controller().turn_on() test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


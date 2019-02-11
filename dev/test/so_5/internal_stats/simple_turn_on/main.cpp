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

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

void
init( so_5::environment_t & env )
{
	env.stats_controller().turn_on();

	class actor_t final : public so_5::agent_t
	{
	public :
		using so_5::agent_t::agent_t;

		void so_evt_start() override
		{
			so_environment().stop();
		}
	};

	env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< actor_t >();
		} );
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


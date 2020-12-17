/*
 * A simple test for wrapped_env_t.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::wrapped_env_t env{
					[]( auto & ) {},
					[]( auto & params ) {
						params.turn_work_thread_activity_tracking_on();
					}
				};

				ensure_or_die(
						so_5::work_thread_activity_tracking_t::on ==
								env.environment().work_thread_activity_tracking(),
						"work_thread_activity_tracking should be 'on' at this point" );

				std::cout << "Print from primary working thread" << std::endl;
			},
			20,
			"simple test for wrapped_env_t" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


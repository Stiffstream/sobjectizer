/*
 * A simple test for wrapped_env_t.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::wrapped_env_t env;

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


/*
 * A test for adding a coop to wrapped_env_t after start.
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

				std::cout << "Before adding a coop" << std::endl;

				env.environment().introduce_coop( []( so_5::coop_t & coop ) {
					struct hello_sig : public so_5::signal_t {};

					auto a = coop.define_agent();
					a.on_start( [a] {
							so_5::send_delayed< hello_sig >( a,
									std::chrono::milliseconds( 25 ) );
						} );
					a.event< hello_sig >( a, [a] {
							std::cout << "Hello for agent" << std::endl;
							so_5::send_delayed< hello_sig >( a,
									std::chrono::milliseconds( 100 ) );
						} );
					a.on_finish( [] {
							std::cout << "Bye from agent" << std::endl;
						} );
				} );

				std::cout << "Coop added" << std::endl;

				std::this_thread::sleep_for( std::chrono::milliseconds(250) );

				std::cout << "Finishing test..." << std::endl;
			},
			20,
			"test for add a coop to wrapped_env_t" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


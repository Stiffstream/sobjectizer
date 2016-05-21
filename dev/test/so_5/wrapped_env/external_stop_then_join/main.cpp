/*
 * A test for stopping wrapped_env_t.
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
				so_5::wrapped_env_t sobj{
					[]( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
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
						} );
					}
				};

				std::this_thread::sleep_for( std::chrono::milliseconds(250) );

				sobj.stop();

				std::cout << "Stop signal is sent" << std::endl;

				sobj.join();

				std::cout << "Joined" << std::endl;
			},
			20,
			"test for stopping wrapped_env_t" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


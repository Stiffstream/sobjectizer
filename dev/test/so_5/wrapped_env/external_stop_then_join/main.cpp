/*
 * A test for stopping wrapped_env_t.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

using namespace std::chrono_literals;

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
							class actor_t final : public so_5::agent_t
							{
								struct hello_sig : public so_5::signal_t {};
							public :
								using so_5::agent_t::agent_t;

								void so_evt_start() override
								{
									so_subscribe_self().event(
										[this](mhood_t<hello_sig>) {
											std::cout << "Hello for agent" << std::endl;
											so_5::send_delayed< hello_sig >( *this, 100ms );
										} );

									so_5::send_delayed< hello_sig >( *this, 25ms );
								}
							};

							coop.make_agent< actor_t >();
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


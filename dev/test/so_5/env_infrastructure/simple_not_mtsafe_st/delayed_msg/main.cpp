/*
 * A test for simple_not_mtsafe_st_env_infastructure with one simple agent
 * and delayed stop message.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						struct stop {};

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							auto a = coop.define_agent();
							a.on_start(
								[a] {
									so_5::send_delayed< stop >(
											a,
											std::chrono::milliseconds(250) );
								} );
							a.event( a, []( const stop & ) {
									// Do nothing.
									// Environment will be stopped anyway.
								} );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );
			},
			5,
			"simple agent with delayed stop message" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


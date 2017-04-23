/*
 * A test for simple_mtsafe_st_env_infastructure with one simple agent.
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
				bool agent_started = false;
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						struct stop {};

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							auto a = coop.define_agent();
							a.on_start(
								[a, &agent_started]{
									agent_started = true;
									so_5::send< stop >(a);
								} );
							a.event( a, [&coop]( const stop & ) {
									coop.deregister_normally();
								} );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );

				UT_CHECK_CONDITION( agent_started );
			},
			5,
			"simple agent" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


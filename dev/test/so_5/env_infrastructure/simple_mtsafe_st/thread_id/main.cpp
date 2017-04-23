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
				const auto this_thread_id = so_5::query_current_thread_id();
				so_5::current_thread_id_t agent_thread_id;

				so_5::launch(
					[&]( so_5::environment_t & env ) {
						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							auto a = coop.define_agent();
							a.on_start(
								[&]{
									agent_thread_id = so_5::query_current_thread_id();
									coop.deregister_normally();
								} );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );

				std::cout << "this_thread_id: " << this_thread_id
						<< ", agent_thread_id: " << agent_thread_id
						<< std::endl;

				UT_CHECK_CONDITION( this_thread_id == agent_thread_id );
			},
			5,
			"thread id check" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


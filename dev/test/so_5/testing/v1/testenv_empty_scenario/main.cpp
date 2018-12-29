#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

UT_UNIT_TEST( empty_scenario_with_one_agent )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			env.environment().introduce_coop( [](so_5::coop_t & coop) {
					coop.define_agent().on_start( [] {
							std::cout << "Hello, World!" << std::endl;
						} );
				} );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die(
					tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

UT_UNIT_TEST( empty_scenario_without_agents )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die(
					tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

UT_UNIT_TEST( one_agent_no_call_to_run_for )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			env.environment().introduce_coop( [](so_5::coop_t & coop) {
					coop.define_agent().on_start( [] {
							std::cout << "Hello, World!" << std::endl;
						} );
				} );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( empty_scenario_with_one_agent )
	UT_RUN_UNIT_TEST( empty_scenario_without_agents )

	UT_RUN_UNIT_TEST( one_agent_no_call_to_run_for )

	return 0;
}


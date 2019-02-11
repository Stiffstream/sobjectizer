#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

UT_UNIT_TEST( empty_scenario_with_one_agent )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			env.environment().introduce_coop( [](so_5::coop_t & coop) {
					class actor_t final : public so_5::agent_t
					{
					public:
						using so_5::agent_t::agent_t;

						void so_evt_start() override
						{
							std::cout << "Hello, World!" << std::endl;
						}
					};

					coop.make_agent< actor_t >();
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
					class actor_t final : public so_5::agent_t
					{
					public:
						using so_5::agent_t::agent_t;

						void so_evt_start() override
						{
							std::cout << "Hello, World!" << std::endl;
						}
					};

					coop.make_agent< actor_t >();
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


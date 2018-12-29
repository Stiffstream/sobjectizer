/*
 * A simple test for testing::v1::test_env_t.
 */

#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

struct hello final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &test_agent_t::on_hello );
	}

private :
	void
	on_hello( mhood_t<hello> )
	{
		std::cout << "Hello!" << std::endl;
	}
};

UT_UNIT_TEST( impact_as_lambda )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			env.scenario().define_step( "hello_received" )
				.impact( [test_agent] {
							so_5::send< hello >( *test_agent );
						} )
				.when( *test_agent & tests::reacts_to< hello >() );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die(
					tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

UT_UNIT_TEST( impact_as_message )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			env.scenario().define_step( "hello_received" )
				.impact< hello >( *test_agent )
				.when( *test_agent & tests::reacts_to< hello >() );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die(
					tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( impact_as_lambda )
	UT_RUN_UNIT_TEST( impact_as_message )

	return 0;
}


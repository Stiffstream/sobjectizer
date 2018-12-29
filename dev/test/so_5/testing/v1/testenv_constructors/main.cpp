#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

class test_agent_t final : public so_5::agent_t
{
public :
	struct hello final : public so_5::signal_t {};

	test_agent_t(
		context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( [](mhood_t<hello>) {} );
	}
};

UT_UNIT_TEST( env_params_tuner )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env{
				[]( so_5::environment_params_t & params ) {
					params.message_delivery_tracer(
							so_5::msg_tracing::std_cout_tracer() );
				} };

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			env.scenario().define_step( "hello" )
				.impact( [test_agent] {
							so_5::send< test_agent_t::hello >( *test_agent );
						} )
				.when( *test_agent & tests::reacts_to< test_agent_t::hello >() );

			env.scenario().run_for( std::chrono::milliseconds(500) );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( prepared_params )
{
	const auto params_maker = [] {
		so_5::environment_params_t params;
		params.message_delivery_tracer(
				so_5::msg_tracing::std_cout_tracer() );
		return params;
	};

	run_with_time_limit(
		[=]()
		{
			tests::testing_env_t env{ params_maker() };

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			env.scenario().define_step( "hello" )
				.impact( [test_agent] {
							so_5::send< test_agent_t::hello >( *test_agent );
						} )
				.when( *test_agent & tests::reacts_to< test_agent_t::hello >() );

			env.scenario().run_for( std::chrono::milliseconds(500) );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( env_params_tuner );
	UT_RUN_UNIT_TEST( prepared_params );

	return 0;
}


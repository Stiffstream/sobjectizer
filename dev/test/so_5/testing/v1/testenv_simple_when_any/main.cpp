/*
 * A simple test for testing::v1::test_env_t.
 */

#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

struct hello_1 final : public so_5::signal_t {};
struct hello_2 final : public so_5::signal_t {};
struct dummy final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &test_agent_t::on_hello_1 );
		so_subscribe_self().event( &test_agent_t::on_hello_2 );
	}

private :
	void
	on_hello_1( mhood_t<hello_1> )
	{
		std::cout << "Hello(1)!" << std::endl;
	}

	void
	on_hello_2( mhood_t<hello_2> )
	{
		std::cout << "Hello(2)!" << std::endl;
	}
};

template< typename Step >
void
setup_triggers(
	so_5::agent_t & agent,
	Step && step )
{
	step.when_any(
			agent & tests::reacts_to< hello_1 >(),
			agent & tests::reacts_to< hello_2 >(),
			agent & tests::ignores< dummy >() );
}

UT_UNIT_TEST( send_hello_1 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			setup_triggers(
					*test_agent,
					env.scenario().define_step( "hello_received" )
							.impact( [test_agent] {
										so_5::send< hello_1 >( *test_agent );
									} ) );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

UT_UNIT_TEST( send_hello_2 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			setup_triggers(
					*test_agent,
					env.scenario().define_step( "hello_received" )
						.impact( [test_agent] {
									so_5::send< hello_2 >( *test_agent );
								} ) );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

UT_UNIT_TEST( send_dummy )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			setup_triggers(
					*test_agent,
					env.scenario().define_step( "dummy_ignored" )
						.impact( [test_agent] {
									so_5::send< dummy >( *test_agent );
								} ) );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( send_hello_1 );
	UT_RUN_UNIT_TEST( send_hello_2 );
	UT_RUN_UNIT_TEST( send_dummy );

	return 0;
}


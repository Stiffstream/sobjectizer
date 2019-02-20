#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std::chrono_literals;

namespace tests = so_5::experimental::testing::v1;

struct hello_1 final : public so_5::signal_t {};
struct hello_2 final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t(
		context_t ctx,
		std::chrono::steady_clock::duration pause )
		:	so_5::agent_t( std::move(ctx) )
		,	m_pause( pause )
	{
		so_subscribe_self().event( &test_agent_t::on_hello_1 );
		so_subscribe_self().event( &test_agent_t::on_hello_2 );
	}

private :
	const std::chrono::steady_clock::duration m_pause;

	void
	on_hello_1( mhood_t<hello_1> )
	{
		so_5::send_delayed<hello_2>( *this, m_pause );
	}

	void
	on_hello_2( mhood_t<hello_2> )
	{
	}
};

void
define_hello_1_step(
	tests::testing_env_t & env,
	so_5::agent_t & agent )
{
	env.scenario().define_step( "hello_1" )
		.impact( [&agent] { so_5::send< hello_1 >( agent ); } )
		.when( agent & tests::reacts_to< hello_1 >() );
}

UT_UNIT_TEST( not_before_1 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 200ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_before( 500ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_NE( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_before_2 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 700ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_before( 300ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_after_1 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 200ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_after( 400ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_after_2 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 500ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_after( 250ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_NE( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_before_not_after_1 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 200ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( 400ms ),
						tests::not_after( 600ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_NE( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_before_not_after_2 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 400ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( 150ms ),
						tests::not_after( 200ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_NE( tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( not_before_not_after_3 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >( 250ms );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( 100ms ),
						tests::not_after( 500ms ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( not_before_1 );
	UT_RUN_UNIT_TEST( not_before_2 );

	UT_RUN_UNIT_TEST( not_after_1 );
	UT_RUN_UNIT_TEST( not_after_2 );

	UT_RUN_UNIT_TEST( not_before_not_after_1 );
	UT_RUN_UNIT_TEST( not_before_not_after_2 );
	UT_RUN_UNIT_TEST( not_before_not_after_3 );

	return 0;
}


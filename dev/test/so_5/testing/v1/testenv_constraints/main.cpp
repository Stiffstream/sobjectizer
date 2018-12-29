#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(200) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_before( std::chrono::milliseconds(500) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(700) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_before( std::chrono::milliseconds(300) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(200) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_after( std::chrono::milliseconds(400) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(500) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints( tests::not_after( std::chrono::milliseconds(250) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(200) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( std::chrono::milliseconds(400) ),
						tests::not_after( std::chrono::milliseconds(600) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(400) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( std::chrono::milliseconds(150) ),
						tests::not_after( std::chrono::milliseconds(200) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >(
							std::chrono::milliseconds(250) );
				} );

			define_hello_1_step( env, *test_agent );

			env.scenario().define_step( "hello_2" )
				.constraints(
						tests::not_before( std::chrono::milliseconds(100) ),
						tests::not_after( std::chrono::milliseconds(500) ) )
				.when( *test_agent & tests::reacts_to<hello_2>() );

			env.scenario().run_for( std::chrono::milliseconds(1000) );

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


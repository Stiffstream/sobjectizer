#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std::chrono_literals;

namespace so5_tests = so_5::experimental::testing::v1;

namespace test
{

constexpr std::chrono::milliseconds expected_delay{ 200 };

struct start final : public so_5::signal_t {};

class test_agent_no_delay_t final : public so_5::agent_t
{
public :
	test_agent_no_delay_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( []( mhood_t<start> ) { /* Do nothing */ } );
	}
};

class test_agent_with_delay_t final : public so_5::agent_t
{
	so_5::state_t st_working{ this, "working" };

public :
	test_agent_with_delay_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
	}

	void
	so_define_agent() override
	{
		this >>= st_working;

		so_subscribe_self().in( st_working ).event( []( mhood_t<start> ) {
				std::this_thread::sleep_for( expected_delay + 10ms );
			} );
	}
};

} /* namespace test */

using namespace test;

UT_UNIT_TEST( no_delay_in_agent )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_no_delay_t >();
				} );

			env.scenario().define_step( "start" )
				.impact( [test_agent] { so_5::send< start >( *test_agent ); } )
				.when( *test_agent
						& so5_tests::reacts_to< start >()
						& so5_tests::wait_event_handler_completion() )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ(
					true,
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) < expected_delay );
		},
		5 );
}

UT_UNIT_TEST( no_wait_event_handler_completion )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_with_delay_t >();
				} );

			env.scenario().define_step( "start" )
				.impact( [test_agent] { so_5::send< start >( *test_agent ); } )
				.when( *test_agent
						& so5_tests::reacts_to< start >() )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ(
					true,
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) < expected_delay );
		},
		5 );
}

UT_UNIT_TEST( wait_event_handler_completion )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_with_delay_t >();
				} );

			env.scenario().define_step( "start" )
				.impact( [test_agent] { so_5::send< start >( *test_agent ); } )
				.when( *test_agent
						& so5_tests::reacts_to< start >()
						& so5_tests::wait_event_handler_completion() )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ(
					true,
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) >= expected_delay );
		},
		5 );
}

UT_UNIT_TEST( wait_and_store_state_name_1 )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_with_delay_t >();
				} );

			env.scenario().define_step( "start" )
				.impact( [test_agent] { so_5::send< start >( *test_agent ); } )
				.when( *test_agent
						& so5_tests::reacts_to< start >()
						& so5_tests::wait_event_handler_completion()
						& so5_tests::store_state_name( "agent" ) )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ(
					true,
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) >= expected_delay );
			UT_CHECK_EQ( "working", env.scenario().stored_state_name( "start", "agent" ) );
		},
		5 );
}

UT_UNIT_TEST( wait_and_store_state_name_2 )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_with_delay_t >();
				} );

			env.scenario().define_step( "start" )
				.impact( [test_agent] { so_5::send< start >( *test_agent ); } )
				.when( *test_agent
						& so5_tests::reacts_to< start >()
						& so5_tests::store_state_name( "agent" )
						& so5_tests::wait_event_handler_completion() )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ(
					true,
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) >= expected_delay );
			UT_CHECK_EQ( "working", env.scenario().stored_state_name( "start", "agent" ) );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( no_delay_in_agent )
	UT_RUN_UNIT_TEST( no_wait_event_handler_completion )
	UT_RUN_UNIT_TEST( wait_event_handler_completion )
	UT_RUN_UNIT_TEST( wait_and_store_state_name_1 )
	UT_RUN_UNIT_TEST( wait_and_store_state_name_2 )

	return 0;
}


#include "../dummy_yield.hpp"

#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <tuple>

using namespace std::chrono_literals;

namespace so5_tests = so_5::experimental::testing::v1;

namespace test
{

constexpr std::chrono::milliseconds expected_delay{ 50 };

struct start final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx )
		:	so_5::agent_t( ctx + so_5::enable_cpp_coro() )
	{
		so_subscribe_self().event(
			[this]( mhood_t<start> ) -> so_5::cpp_coro::task_t< void > {
				co_await dummy_yield( so_environment(), expected_delay );
			} );
	}
};

class noop_agent_t final : public so_5::agent_t
{
public:
	noop_agent_t( context_t ctx )
		: so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( []( mhood_t<start> ) -> void {
			} );
	}
};

class time_fixer_t final : public so_5::agent_t
{
	std::chrono::steady_clock::time_point & m_receiver;

public:
	time_fixer_t(
		context_t ctx,
		std::chrono::steady_clock::time_point & receiver )
		: so_5::agent_t{ std::move(ctx) }
		, m_receiver{ receiver }
	{
		so_subscribe_self().event(
			[this]( mhood_t<start> ) -> void {
				m_receiver = std::chrono::steady_clock::now();
			} );
	}
};

class state_changer_t final : public so_5::agent_t
{
	state_t st_initial{ this, "initial" };
	state_t st_before{ this, "before" };
	state_t st_after{ this, "after" };

public :
	state_changer_t( context_t ctx )
		:	so_5::agent_t( ctx + so_5::enable_cpp_coro() )
	{
	}

	void
	so_define_agent() override
	{
		this >>= st_initial;

		so_subscribe_self()
			.in( st_initial )
			.event(
			[this]( mhood_t<start> ) -> so_5::cpp_coro::task_t< void > {
				this >>= st_before;

				co_await dummy_yield( so_environment(), expected_delay );

				this >>= st_after;
			} );
	}
};

} /* namespace test */

using namespace test;

UT_UNIT_TEST( basic )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< test_agent_t >();
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
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
							>= expected_delay );
		},
		5 );
}

UT_UNIT_TEST( basic_with_state_change )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					return coop.make_agent< state_changer_t >();
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
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
							>= expected_delay );
			UT_CHECK_EQ(
					"after",
					env.scenario().stored_state_name( "start", "agent" ) );
		},
		5 );
}

UT_UNIT_TEST( basic_with_continuation )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			std::chrono::steady_clock::time_point time_of_third =
					std::chrono::steady_clock::now() + expected_delay;

			auto [first, second, third] =
				env.environment().introduce_coop(
					[&](so_5::coop_t & coop) {
						auto * first = coop.make_agent< test_agent_t >();
						auto * second = coop.make_agent< noop_agent_t >();
						auto * third = coop.make_agent< time_fixer_t >(
								time_of_third );

						return std::make_tuple( first, second, third );
					} );

			env.scenario().define_step( "start" )
				.impact( [first] { so_5::send< start >( *first ); } )
				.when( *first
						& so5_tests::reacts_to< start >()
						& so5_tests::wait_event_handler_completion() )
				;

			env.scenario().define_step( "noop" )
				.impact( [second] { so_5::send< start >( *second ); } )
				.when( *second & so5_tests::reacts_to< start >() )
				;

			env.scenario().define_step( "time_fix" )
				.impact( [third] { so_5::send< start >( *third ); } )
				.when( *third & so5_tests::reacts_to< start >() )
				;

			const auto t1 = std::chrono::steady_clock::now();
			env.scenario().run_for( 1000ms );
			const auto t2 = std::chrono::steady_clock::now();

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_GE(
					std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1),
					expected_delay );
			UT_CHECK_LT(
					std::chrono::duration_cast<std::chrono::milliseconds>(
							time_of_third - t1 ),
					expected_delay );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( basic )
	UT_RUN_UNIT_TEST( basic_with_state_change )
	UT_RUN_UNIT_TEST( basic_with_continuation )

	return 0;
}


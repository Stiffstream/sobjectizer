#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

struct ul_started final : public so_5::message_t {};
struct ul_stopped final : public so_5::message_t {};

class test_agent_t final : public so_5::agent_t
{
	bool m_ul_allowed{ true };

public :

	test_agent_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{}

	void
	so_define_agent()
	{
		so_subscribe_self()
			.event( &test_agent_t::evt_ul_started )
			.event( &test_agent_t::evt_ul_stopped )
			;

		so_set_delivery_filter( so_direct_mbox(),
				[this]( const ul_started & ) {
					std::cout << "delivery_filter invoked" << std::endl;
					return this->m_ul_allowed;
				} );
	}

private :
	void
	evt_ul_started( mhood_t<ul_started> )
	{
		std::cout << "ul_started" << std::endl;
		m_ul_allowed = false;
	}

	void
	evt_ul_stopped( mhood_t<ul_stopped> )
	{
		std::cout << "ul_stopped" << std::endl;
		m_ul_allowed = true;
	}
};

using namespace std::chrono_literals;

UT_UNIT_TEST( issue_108 )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env{
#if 0
				 [](so_5::environment_params_t & params) {
					// Turn message delivery tracing on.
					params.message_delivery_tracer(
						so_5::msg_tracing::std_cout_tracer());
				 }
#endif
			 };

			so_5::agent_t * test_agent = env.environment().introduce_coop(
					[](so_5::coop_t & coop) {
						return coop.make_agent< test_agent_t >();
					} );

			env.scenario().define_step( "ul_started #1" )
				.impact< ul_started >( *test_agent )
				.when( *test_agent & tests::reacts_to< ul_started >() );

			env.scenario().define_step( "ul_stopped #1" )
				.impact< ul_stopped >( *test_agent )
				.when( *test_agent & tests::reacts_to< ul_stopped >() );

			env.scenario().define_step( "ul_started #2" )
				.impact( [d = test_agent->so_direct_mbox()]()
					{
						so_5::send_delayed< ul_started >( d, 100ms );
					})
				.when( *test_agent & tests::reacts_to< ul_started >() );

			env.scenario().define_step( "ul_started #3" )
				.impact( [d = test_agent->so_direct_mbox()]()
					{
						so_5::send_delayed< ul_started >( d, 100ms );
					})
				.when( *test_agent & tests::ignores< ul_started >() );

			env.scenario().run_for( std::chrono::milliseconds(400) );

			UT_CHECK_NE( tests::completed(), env.scenario().result() );

			std::ostringstream ss;
			ss << env.scenario().result();

			UT_CHECK_EQ( ss.str(), "[timed_out,{preactivated step:ul_started #3;}]" );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( issue_108 )

	return 0;
}


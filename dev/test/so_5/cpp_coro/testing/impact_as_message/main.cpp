#include "../dummy_yield.hpp"

#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace so5_tests = so_5::experimental::testing::v1;

using namespace std::chrono_literals;

namespace test
{

struct hello final : public so_5::signal_t {};

struct so_msg final : public so_5::message_t {
	int m_value;

	so_msg( int v ) : m_value(v) {}
};

struct usr_msg final {
	int m_value;
};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx )
		:	so_5::agent_t( ctx + so_5::enable_cpp_coro() )
	{
		so_subscribe_self()
			.event( &test_agent_t::on_hello )
			.event( &test_agent_t::on_so_msg_1 )
			.event( &test_agent_t::on_so_msg_2 )
			.event( &test_agent_t::on_usr_msg_1 )
			.event( &test_agent_t::on_usr_msg_2 )
			;
	}

private :
	so_5::cpp_coro::task_t< void >
	on_hello( mhood_t<hello> ) {
		co_await dummy_yield( so_environment(), 5ms );
std::cout << "on hello resumed" << std::endl;
	}

	void on_so_msg_1( mhood_t<so_msg> ) {}
	void on_so_msg_2( mutable_mhood_t<so_msg> ) {}
	void on_usr_msg_1( mhood_t<usr_msg> ) {}
	void on_usr_msg_2( mutable_mhood_t<usr_msg> ) {}
};

template< typename Msg, typename... Args >
void
define_step(
	so5_tests::testing_env_t & env,
	std::string step_name,
	so_5::agent_t & agent,
	Args &&... args )
{
	env.scenario().define_step( std::move(step_name) )
		.impact< Msg >( agent, std::forward<Args>(args)... )
		.when( agent & so5_tests::reacts_to< Msg >() );
}

} /* namespace test */

using namespace test;

UT_UNIT_TEST( impact_as_message )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env{
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}
				};

			so_5::agent_t * test_agent = env.environment().introduce_coop(
					[](so_5::coop_t & coop) {
						return coop.make_agent< test_agent_t >();
					} );

			define_step< hello >( env, "hello", *test_agent );
			define_step< so_msg >( env, "so_msg", *test_agent, 1 );
			define_step< so_5::mutable_msg<so_msg> >(
					env, "mutable<so_msg>", *test_agent, 2 );
			define_step< usr_msg >( env, "usr_msg", *test_agent, 1 );
			define_step< so_5::mutable_msg<usr_msg> >(
					env, "mutable<usr_msg>", *test_agent, 2 );

			env.scenario().run_for( std::chrono::seconds(1) );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( impact_as_message )

	return 0;
}


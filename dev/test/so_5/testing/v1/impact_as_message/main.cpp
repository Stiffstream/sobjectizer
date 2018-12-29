#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

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
		:	so_5::agent_t( std::move(ctx) )
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
	void on_hello( mhood_t<hello> ) {}
	void on_so_msg_1( mhood_t<so_msg> ) {}
	void on_so_msg_2( mutable_mhood_t<so_msg> ) {}
	void on_usr_msg_1( mhood_t<usr_msg> ) {}
	void on_usr_msg_2( mutable_mhood_t<usr_msg> ) {}
};

template< typename Msg, typename... Args >
void
define_step(
	tests::testing_env_t & env,
	std::string step_name,
	so_5::agent_t & agent,
	Args &&... args )
{
	env.scenario().define_step( std::move(step_name) )
		.impact< Msg >( agent, std::forward<Args>(args)... )
		.when( agent & tests::reacts_to< Msg >() );
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

			define_step< hello >( env, "hello", *test_agent );
			define_step< so_msg >( env, "so_msg", *test_agent, 1 );
			define_step< so_5::mutable_msg<so_msg> >(
					env, "mutable<so_msg>", *test_agent, 2 );
			define_step< usr_msg >( env, "usr_msg", *test_agent, 1 );
			define_step< so_5::mutable_msg<usr_msg> >(
					env, "mutable<usr_msg>", *test_agent, 2 );

			env.scenario().run_for( std::chrono::seconds(1) );

			UT_CHECK_EQ( tests::completed(), env.scenario().result() );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( impact_as_message )

	return 0;
}


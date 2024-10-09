#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace so5_tests = so_5::experimental::testing::v1;

namespace test
{

struct so_msg final : public so_5::message_t {
	int m_value;

	so_msg( int v ) : m_value(v) {}
};

struct usr_msg final {
	int m_value;
};

class agent_that_handles_messages_t final : public so_5::agent_t
{
public :
	agent_that_handles_messages_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self()
			.event( &agent_that_handles_messages_t::on_so_msg_1 )
			.event( &agent_that_handles_messages_t::on_so_msg_2 )
			.event( &agent_that_handles_messages_t::on_usr_msg_1 )
			.event( &agent_that_handles_messages_t::on_usr_msg_2 )
			;
	}

private :
	void on_so_msg_1( mhood_t<so_msg> ) {}
	void on_so_msg_2( mutable_mhood_t<so_msg> ) {}
	void on_usr_msg_1( mhood_t<usr_msg> ) {}
	void on_usr_msg_2( mutable_mhood_t<usr_msg> ) {}
};

const std::string inspection_result_tag{ "inspection-result" };

template< typename Msg, typename... Args >
void
define_reacts_to_step(
	so5_tests::testing_env_t & env,
	std::string step_name,
	so_5::agent_t & agent,
	int expected_value,
	Args &&... args )
{
	using message_to_receive_type = typename so_5::message_payload_type< Msg >::payload_type;

	env.scenario().define_step( std::move(step_name) )
		.impact< Msg >( agent, std::forward<Args>(args)... )
		.when( agent & so5_tests::reacts_to< Msg >()
				& so5_tests::inspect_msg( inspection_result_tag,
						[expected_value]( const message_to_receive_type & msg ) -> std::string {
							return expected_value == msg.m_value
									? "OK"
									: "MISSMATCH";
						} )
		);
}

class agent_that_ignores_messages_t final : public so_5::agent_t
{
	const state_t st_inactive{ this, "inactive" };

public :
	agent_that_ignores_messages_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().in( st_inactive )
			.event( &agent_that_ignores_messages_t::on_so_msg_1 )
			.event( &agent_that_ignores_messages_t::on_so_msg_2 )
			.event( &agent_that_ignores_messages_t::on_usr_msg_1 )
			.event( &agent_that_ignores_messages_t::on_usr_msg_2 )
			;
	}

private :
	void on_so_msg_1( mhood_t<so_msg> ) {}
	void on_so_msg_2( mutable_mhood_t<so_msg> ) {}
	void on_usr_msg_1( mhood_t<usr_msg> ) {}
	void on_usr_msg_2( mutable_mhood_t<usr_msg> ) {}
};

template< typename Msg, typename... Args >
void
define_ignores_step(
	so5_tests::testing_env_t & env,
	std::string step_name,
	so_5::agent_t & agent,
	int expected_value,
	Args &&... args )
{
	using message_to_receive_type = typename so_5::message_payload_type< Msg >::payload_type;

	env.scenario().define_step( std::move(step_name) )
		.impact< Msg >( agent, std::forward<Args>(args)... )
		.when( agent & so5_tests::ignores< Msg >()
				& so5_tests::inspect_msg( inspection_result_tag,
						[expected_value]( const message_to_receive_type & msg ) -> std::string {
							return expected_value == msg.m_value
									? "OK"
									: "MISSMATCH";
						} )
		);
}

} /* namespace test */

using namespace test;

UT_UNIT_TEST( inspect_when_reacts )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
					[](so_5::coop_t & coop) {
						return coop.make_agent< agent_that_handles_messages_t >();
					} );

			define_reacts_to_step< so_msg >( env, "so_msg", *test_agent, 1, 1 );
			define_reacts_to_step< so_5::mutable_msg<so_msg> >(
					env, "mutable<so_msg>", *test_agent, 2, 2 );
			define_reacts_to_step< usr_msg >( env, "usr_msg", *test_agent, 1, 1 );
			define_reacts_to_step< so_5::mutable_msg<usr_msg> >(
					env, "mutable<usr_msg>", *test_agent, 2, 2 );

			env.scenario().run_for( std::chrono::seconds(1) );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"so_msg", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"mutable<so_msg>", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"usr_msg", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"mutable<usr_msg>", inspection_result_tag ) );
		},
		5 );
}

UT_UNIT_TEST( inspect_when_ignores )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::agent_t * test_agent = env.environment().introduce_coop(
					[](so_5::coop_t & coop) {
						return coop.make_agent< agent_that_ignores_messages_t >();
					} );

			define_ignores_step< so_msg >( env, "so_msg", *test_agent, 1, 1 );
			define_ignores_step< so_5::mutable_msg<so_msg> >(
					env, "mutable<so_msg>", *test_agent, 2, 2 );
			define_ignores_step< usr_msg >( env, "usr_msg", *test_agent, 1, 1 );
			define_ignores_step< so_5::mutable_msg<usr_msg> >(
					env, "mutable<usr_msg>", *test_agent, 2, 2 );

			env.scenario().run_for( std::chrono::seconds(1) );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"so_msg", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"mutable<so_msg>", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"usr_msg", inspection_result_tag ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
					"mutable<usr_msg>", inspection_result_tag ) );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( inspect_when_reacts )
	UT_RUN_UNIT_TEST( inspect_when_ignores )

	return 0;
}


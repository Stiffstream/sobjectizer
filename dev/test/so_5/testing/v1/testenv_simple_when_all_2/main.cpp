/*
 * A simple test for testing::v1::test_env_t.
 */

#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

namespace tests = so_5::experimental::testing::v1;

struct msg_take final : public so_5::message_t
{
	const so_5::mbox_t m_reply_to;

	msg_take( so_5::mbox_t reply_to )
		:	m_reply_to{ std::move(reply_to) }
		{}
};

struct msg_taken final : public so_5::signal_t {};

class first_t final : public so_5::agent_t
{
	const state_t st_taken{ this, "taken" };

public :
	first_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [this](mhood_t<msg_take> cmd) {
			this >>= st_taken;
			so_5::send<msg_taken>( cmd->m_reply_to );
		} );
	}
};

class second_t final : public so_5::agent_t
{
public :
	second_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [](mhood_t<msg_taken>) {} );
	}
};

int
main()
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * first{};
			so_5::agent_t * second{};

			env.environment().introduce_coop(
				[&](so_5::coop_t & coop) {
					first = coop.make_agent< first_t >();
					second = coop.make_agent< second_t >();
				} );

			env.scenario().define_step( "test" )
				.impact( [first, second] {
							so_5::send< msg_take >( *first, second->so_direct_mbox() );
						} )
				.when_all(
						*first & tests::reacts_to< msg_take >()
								& tests::store_state_name( "first" ),
						*second & tests::reacts_to< msg_taken >() );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );

	return 0;
}


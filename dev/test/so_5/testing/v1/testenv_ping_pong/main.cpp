#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

namespace tests = so_5::experimental::testing::v1;

struct ping final : public so_5::signal_t {};
struct pong final : public so_5::signal_t {};

class pinger_t final : public so_5::agent_t
{
public :
	pinger_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [](mhood_t<pong>) {
			std::cout << "pong!" << std::endl;
		} );
	}

	void
	set_target( const so_5::mbox_t & m ) { m_target = m; }

	void
	so_evt_start() override
	{
		so_5::send< ping >( m_target );
	}

private :
	so_5::mbox_t m_target;
};

class ponger_t final : public so_5::agent_t
{
public :
	ponger_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [this](mhood_t<ping>) {
			std::cout << "ping!" << std::endl;
			so_5::send< pong >( m_target );
		} );
	}

	void
	set_target( const so_5::mbox_t & m ) { m_target = m; }

private :
	so_5::mbox_t m_target;
};

UT_UNIT_TEST( ping_pong )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			pinger_t * pinger{};
			ponger_t * ponger{};
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					pinger = coop.make_agent< pinger_t >();
					ponger = coop.make_agent< ponger_t >();

					pinger->set_target( ponger->so_direct_mbox() );
					ponger->set_target( pinger->so_direct_mbox() );
				} );

			env.scenario().define_step( "ping" )
				.when( *ponger & tests::reacts_to< ping >() );

			env.scenario().define_step( "pong" )
				.when( *pinger & tests::reacts_to< pong >() );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die(
					tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( ping_pong )

	return 0;
}


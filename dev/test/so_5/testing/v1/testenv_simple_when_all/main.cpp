/*
 * A simple test for testing::v1::test_env_t.
 */

#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace tests = so_5::experimental::testing::v1;

struct hello final : public so_5::signal_t {};
struct confirm final : public so_5::signal_t {};
struct dummy final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx, const so_5::mbox_t & broadcast )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe( broadcast ).event( &test_agent_t::on_hello );
		so_subscribe_self().event( &test_agent_t::on_confirm );
	}

private :
	void
	on_hello( mhood_t<hello> )
	{
		so_5::send< confirm >( *this );
	}

	void
	on_confirm( mhood_t<confirm> )
	{
		so_5::send< dummy >( *this );
	}
};

int
main()
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			const auto broadcast = env.environment().create_mbox();

			auto [first, second, third, fourth] = env.environment().introduce_coop(
				so_5::disp::active_obj::make_dispatcher(
						env.environment() ).binder(),
				[&](so_5::coop_t & coop) {
					return std::make_tuple(
							coop.make_agent< test_agent_t >( broadcast ),
							coop.make_agent< test_agent_t >( broadcast ),
							coop.make_agent< test_agent_t >( broadcast ),
							coop.make_agent< test_agent_t >( broadcast ) );
				} );

			env.scenario().define_step( "test" )
				.impact( [broadcast] {
							so_5::send< hello >( broadcast );
						} )
				.when_all(
						*first & tests::reacts_to< hello >(broadcast),
						*second & tests::reacts_to< hello >(broadcast),
						*third & tests::reacts_to< confirm >(),
						*fourth & tests::ignores< dummy >() );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );

	return 0;
}


#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace tests = so_5::experimental::testing::v1;

struct start final : public so_5::signal_t {};

class first_t final : public so_5::agent_t
{
	const state_t st_dummy{ this, "dummy" };

public :
	first_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
	{
		st_dummy.event( [](mhood_t<start>) {} );
	}
};

class second_t final : public so_5::agent_t
{
public :
	second_t( context_t ctx, const so_5::mbox_t & dest )
		: so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [dest](mhood_t<start>) {
			so_5::send<start>( dest );
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		} );
	}
};

int
main()
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			auto [first, second] = env.environment().introduce_coop(
				so_5::disp::active_obj::make_dispatcher(
						env.environment() ).binder(),
				[](so_5::coop_t & coop) {
					auto first_agent = coop.make_agent< first_t >();
					return std::make_tuple(
							first_agent,
							coop.make_agent< second_t >(
									first_agent->so_direct_mbox() ) );
				} );

			env.scenario().define_step( "test" )
				.impact( [s=second] {
							so_5::send< start >( *s );
						} )
				.when_all(
						*first & tests::ignores< start >(),
						*second & tests::reacts_to< start >()
								& tests::store_state_name( "second" ) );

			env.scenario().run_for( std::chrono::seconds(1) );

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );
		},
		5 );

	return 0;
}


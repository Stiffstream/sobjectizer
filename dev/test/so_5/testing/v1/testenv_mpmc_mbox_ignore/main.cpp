#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

namespace tests = so_5::experimental::testing::v1;

struct hello final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
public :
	test_agent_t( context_t ctx )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event( &test_agent_t::on_hello );
	}

private :
	void
	on_hello( mhood_t<hello> )
	{
		std::cout << "Hello!" << std::endl;
	}
};

int
main()
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env;

			so_5::agent_t * test_agent;
			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					test_agent = coop.make_agent< test_agent_t >();
				} );

			const auto broadcast = env.environment().create_mbox();

			env.scenario().define_step( "hello_received" )
				.impact( [broadcast] {
							so_5::send< hello >( broadcast );
						} )
				.when_any(
						*test_agent & tests::reacts_to< hello >( broadcast ),
						*test_agent & tests::ignores< hello >( broadcast ) );

			env.scenario().run_for( std::chrono::milliseconds(250) );

			ensure_or_die( tests::completed() != env.scenario().result(),
					"scenario should not complete its work" );
		},
		5 );

	return 0;
}


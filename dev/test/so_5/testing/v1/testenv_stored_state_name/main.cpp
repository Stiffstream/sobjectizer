#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

namespace tests = so_5::experimental::testing::v1;

struct hello_one final : public so_5::signal_t {};
struct hello_two final : public so_5::signal_t {};

class first_agent_t final : public so_5::agent_t
{
	const state_t st_main{ this, "main" };

public :
	first_agent_t(
		context_t ctx,
		so_5::mbox_t second_mbox )
		:	so_5::agent_t( std::move(ctx) )
		,	m_second_mbox( std::move(second_mbox) )
	{
		so_subscribe_self().event( &first_agent_t::on_hello_one );
	}

private :
	const so_5::mbox_t m_second_mbox;

	void
	on_hello_one( mhood_t<hello_one> )
	{
		so_5::send< hello_two >( m_second_mbox );

		std::cout << "Before sleep..." << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds(250) );
		std::cout << "After sleep..." << std::endl;

		this >>= st_main;
	}
};

class second_agent_t final : public so_5::agent_t
{
public :
	second_agent_t( context_t ctx, const so_5::mbox_t & from )
		:	so_5::agent_t( std::move(ctx) )
	{
		so_subscribe( from ).event( &second_agent_t::on_hello_two );
	}

private :
	void
	on_hello_two( mhood_t<hello_two> ) {}
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

			auto second_mbox = env.environment().create_mbox();

			env.environment().introduce_coop(
				so_5::disp::active_obj::create_private_disp(
						env.environment() )->binder(),
				[&](so_5::coop_t & coop) {
					first = coop.make_agent< first_agent_t >( second_mbox );
					second = coop.make_agent< second_agent_t >( second_mbox );
				} );

			env.scenario().define_step( "hello_one_received" )
				.impact( [first] {
							so_5::send< hello_one >( *first );
						} )
				.when( *first
						& tests::reacts_to< hello_one >()
						& tests::store_state_name( "first" ) );

			env.scenario().define_step( "hello_two_received" )
				.when( *second & tests::reacts_to< hello_two >( second_mbox ) );	

			using clock = std::chrono::steady_clock;

			const auto started_at = clock::now();

			env.scenario().run_for( std::chrono::seconds(1) );

			const auto finished_at = clock::now();

			ensure_or_die( tests::completed() == env.scenario().result(),
					"scenario should complete its work" );

			ensure_or_die(
					(finished_at - started_at) >= std::chrono::milliseconds(250),
					"test should take more than 250ms" );

			ensure_or_die(
					"main" == env.scenario().stored_state_name(
							"hello_one_received", "first" ),
					"`main` is expected to be stored" );
		},
		5 );

	return 0;
}


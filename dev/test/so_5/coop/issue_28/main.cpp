/*
 * A unit-test for testing deregistration child coop bound to active_group
 * dispatcher.
 *
 * See more: https://github.com/Stiffstream/sobjectizer/issues/28#issuecomment-840638292
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct completed final : public so_5::signal_t {};

class a_child_t final : public so_5::agent_t
{
	struct die_t final : public so_5::signal_t {};

	so_5::mchain_t m_final_ch;

public:
	a_child_t( context_t ctx, so_5::mchain_t final_ch )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_final_ch{ std::move(final_ch) }
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event(
			[this](mhood_t<die_t>) {
				so_deregister_agent_coop_normally();
			} );
	}

	void so_evt_start() override
	{
		so_5::send< die_t >( *this );
	}

	void so_evt_finish() override
	{
		so_5::send< completed >( m_final_ch );
	}
};

void
run_test_case( unsigned int n_children )
{
	std::cout << "n_children=" << n_children << std::flush;

	so_5::launch( [n_children]( so_5::environment_t & env ) {
		auto final_ch = so_5::create_mchain( env );

		const auto binder = so_5::disp::active_group::make_dispatcher( env )
				.binder( "my_group" );
		auto parent = env.register_coop( env.make_coop(binder) );

		for( unsigned int i{}; i != n_children; ++i )
		{
			auto child = env.make_coop( parent, binder );
			child->make_agent< a_child_t >( final_ch );
			env.register_coop( std::move(child) );
		}

		so_5::receive(
			so_5::from( final_ch ).handle_n( n_children ),
			[]( so_5::mhood_t< completed > ) {}
		);

		env.stop();
	} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]{
				const auto n_children = { 2u, 4u, 8u, 16u, 32u, 48u, 64u };
				for( auto n : n_children )
				{
					run_test_case( n );
					std::cout << "\r" << std::flush;
				}

				std::cout << "test completed" << std::endl;
			},
			20 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


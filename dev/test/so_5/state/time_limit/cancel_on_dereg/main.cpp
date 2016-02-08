/*
 * A test case for canceling time_limit on agent's deregistration.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t first{ this, "first" };

	struct finish : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		first
			.time_limit( std::chrono::seconds{5}, so_default_state() )
			.event< finish >( [this] { so_deregister_agent_coop_normally(); } );
	}

	virtual void
	so_evt_start() override
	{
		this >>= first;
		so_5::send< finish >( *this );
	}
};

class a_pauser_t final : public so_5::agent_t
{
	struct stop : public so_5::signal_t {};

public :
	a_pauser_t( context_t ctx ) : so_5::agent_t{ ctx }
	{
		so_default_state().event< stop >( [this] {
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		std::cout << "Work will be finished after 5s" << std::endl;
		so_5::send_delayed< stop >( *this, std::chrono::seconds{5} );
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						for( std::size_t i = 0u; i != 50000u; ++i )
							env.introduce_coop( []( so_5::coop_t & coop ) {
									coop.make_agent< a_test_t >();
								} );

						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_pauser_t >();
							} );
					},
					[]( so_5::environment_params_t & params ) {
						params.timer_thread( so_5::timer_list_factory() );
					} );
			},
			240,
			"test for canceling time_limit on deregistration" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


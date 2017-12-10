/*
 * A very simple test case for checking operator== and operator!=
 * for agent states.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };
	state_t st_three{ this, "three" };

public :
	a_test_t( context_t ctx ) : so_5::agent_t( std::move(ctx) ) {}

	virtual void so_evt_start() override
	{
		ensure_or_die( st_one == st_one, "expects st_one == st_one" );
		ensure_or_die( st_one != st_two, "expects st_one != st_two" );
		ensure_or_die( st_one != st_three, "expects st_one != st_three" );

		ensure_or_die( st_one != so_current_state(), "st_one is not the current state" );
		this >>= st_one;
		ensure_or_die( st_one == so_current_state(), "st_one is the current state now" );

		so_deregister_agent_coop_normally();
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
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >();
							} );
					}/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


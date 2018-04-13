/*
 * A test for checking the value of so_current_state() in
 * on_enter/on_exit state handlers.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t st_top_1{ this, "top_1" };
	state_t st_child_1_1{ initial_substate_of{ st_top_1 }, "child_1" };
	state_t st_child_1_2{ substate_of{ st_top_1 }, "child_2" };

	state_t st_child_1_1_1{ initial_substate_of{ st_child_1_1 }, "1" };
	state_t st_child_1_1_2{ substate_of{ st_child_1_1 }, "2" };
	state_t st_child_1_1_3{ substate_of{ st_child_1_1 }, "3" };

	state_t st_top_2{ this, "top_2" };
	state_t st_child_2_1{ initial_substate_of{ st_top_2 }, "child_1" };
	state_t st_child_2_2{ substate_of{ st_top_2 }, "child_2" };

	void
	setup_handlers( state_t & to )
	{
		to.on_enter( [this, &to]{
				ensure( to == so_current_state(),
						to.query_name() + ": on_enter check failed!" ); } );
		to.on_exit( [this, &to]{
				ensure( to == so_current_state(),
						to.query_name() + ": on_exit check failed!" ); } );
	}

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{}

	virtual void
	so_evt_start() override
	{
		std::array<state_t *, 9> states{ {
				&st_top_1, &st_child_1_1, &st_child_1_2,
				&st_child_1_1_1, &st_child_1_1_2, &st_child_1_1_3,
				&st_top_2, &st_child_2_1, &st_child_2_2 } };
		for( auto s : states )
			setup_handlers( *s );

		this >>= st_child_1_1_3;

		ensure( st_child_1_1_3 == so_current_state(),
				"st_child_1_1_3 expected as current state" );

		this >>= st_child_2_2;

		ensure( st_child_2_2 == so_current_state(),
				"st_child_2_2 expected as current state" );

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


/*
 * A very simple test case for clear state history.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t A{ this, "A", deep_history };
	state_t B{ initial_substate_of{ A }, "B", shallow_history };
	state_t C{ initial_substate_of{ B }, "C" };
	state_t D{ substate_of{ B }, "D" };
	state_t E{ this, "E" };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		this >>= A; // The current state is A.B.C.
	}

	virtual void
	so_evt_start() override
	{
		expect_current_state( C );

		this >>= D; // The current state is "A.B.D".
		expect_current_state( D );

		this >>= E; // The current state is "E".
		expect_current_state( E );

		this >>= A; // The current state is "A.B.D" because deep history of A.
		expect_current_state( D );

		this >>= E;
		expect_current_state( E );

		A.clear_history();
		this >>= A; // The current state is "A.B.D" because:
						// B is the initial substate of A and B has shallow history;
						// D is the last active substate of B.
		expect_current_state( D );

		this >>= E;
		expect_current_state( E );

		A.clear_history();
		B.clear_history();
		this >>= A; // The current state is "A.B.C" because:
		            // B is initial substate of A and
						// C is initial substate of B.
		expect_current_state( C );

		so_deregister_agent_coop_normally();
	}

private :
	void
	expect_current_state( const state_t & expected ) const
	{
		ensure( expected == so_current_state(),
			"expected state: " + expected.query_name()
			+ ", actual state: " + so_current_state().query_name() );
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
			20,
			"simple test for clear state history" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


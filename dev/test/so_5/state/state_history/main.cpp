/*
 * A very simple test case for state history.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t s1{ this, "1" };

	state_t s1_1{ initial_substate_of{ s1 }, "1", deep_history };

	state_t s1_1_1{ initial_substate_of{ s1_1 }, "1" };

	state_t s1_1_1_1{ initial_substate_of{ s1_1_1 }, "1", shallow_history };

	state_t s1_1_1_1_1{ initial_substate_of{ s1_1_1_1 }, "1" };
	state_t s1_1_1_1_2{ substate_of{ s1_1_1_1 }, "2" };

	state_t s1_1_1_1_2_1{ initial_substate_of{ s1_1_1_1_2 }, "1" };
	state_t s1_1_1_1_2_2{ substate_of{ s1_1_1_1_2 }, "2" };
	state_t s1_1_1_1_2_3{ substate_of{ s1_1_1_1_2 }, "3" };

	state_t s1_1_1_1_3{ substate_of{ s1_1_1_1 }, "3" };

	state_t s1_1_1_2{ substate_of{ s1_1_1 }, "2" };
	state_t s1_1_1_3{ substate_of{ s1_1_1 }, "3" };

	state_t s1_2{ substate_of{ s1 }, "2" };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		this >>= s1;
	}

	virtual void
	so_evt_start() override
	{
		this >>= s1_1;
		expect_current_state( s1_1_1_1_1 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1_1_1_3;
		expect_current_state( s1_1_1_1_3 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1_1_1;
		expect_current_state( s1_1_1_1_3 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1_1;
		expect_current_state( s1_1_1_1_3 );

		this >>= s1_1_1_1_2_2;
		expect_current_state( s1_1_1_1_2_2 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1;
		expect_current_state( s1_1_1_1_2_2 );

		this >>= s1_1_1_1_3;
		expect_current_state( s1_1_1_1_3 );

		this >>= s1_1_1_1_2_3;
		expect_current_state( s1_1_1_1_2_3 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1_1_1;
		expect_current_state( s1_1_1_1_2_1 );

		this >>= s1_1_1_1_2_3;
		expect_current_state( s1_1_1_1_2_3 );

		this >>= s1_2;
		expect_current_state( s1_2 );

		this >>= s1_1;
		expect_current_state( s1_1_1_1_2_3 );

		so_deregister_agent_coop_normally();
	}

private :
	void
	expect_current_state( const state_t & expected ) const
	{
		if( !(expected == so_current_state()) )
			throw std::runtime_error( "expected state: " + expected.query_name()
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
			"simple test for state history" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


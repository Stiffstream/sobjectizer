/*
 * A very simple test case for checking on_enter and on_exit handlers.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_only_top_level_states_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };
	state_t st_three{ this, "three" };

public :
	a_only_top_level_states_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		st_one
			.on_enter( [this] { m_log += "+1"; } )
			.on_exit( [this] { m_log += "-1"; } );

		st_two
			.on_enter( [this] { m_log += "+2"; } )
			.on_exit( [this] { m_log += "-2"; } );

		st_three
			.on_enter( [this] { m_log += "+3"; } )
			.on_exit( [this] { m_log += "-3"; } );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;
		this >>= st_two;
		this >>= st_three;
		this >>= so_default_state();

		const std::string expected = "+1-1+2-2+3-3";
		if( expected != m_log )
			throw std::runtime_error( expected + " != " + m_log );

		so_deregister_agent_coop_normally();
	}

private :
	std::string m_log;
};

class a_substates_of_one_state_t final : public so_5::agent_t
{
	state_t st_one{ this };
	state_t st_two{ this };

	state_t st_one_1_1{ initial_substate_of{ st_one } };

	state_t st_one_1_1_1{ initial_substate_of{ st_one_1_1 } };
	state_t st_one_1_1_2{ substate_of{ st_one_1_1 } };

	state_t st_one_1_2{ substate_of{ st_one } };

	state_t st_one_1_2_1{ initial_substate_of{ st_one_1_2 } };
	state_t st_one_1_2_2{ substate_of{ st_one_1_2 } };


public :
	a_substates_of_one_state_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		st_one
			.on_enter( [this] { m_log += "+1"; } )
			.on_exit( [this] { m_log += "-1"; } );

		st_one_1_1
			.on_enter( [this] { m_log += "+1_1"; } )
			.on_exit( [this] { m_log += "-1_1"; } );

		st_one_1_1_1 
			.on_enter( [this] { m_log += "+1_1_1"; } )
			.on_exit( [this] { m_log += "-1_1_1"; } );

		st_one_1_1_2 
			.on_enter( [this] { m_log += "+1_1_2"; } )
			.on_exit( [this] { m_log += "-1_1_2"; } );

		st_one_1_2
			.on_enter( [this] { m_log += "+1_2"; } )
			.on_exit( [this] { m_log += "-1_2"; } );

		st_one_1_2_1 
			.on_enter( [this] { m_log += "+1_2_1"; } )
			.on_exit( [this] { m_log += "-1_2_1"; } );

		st_one_1_2_2 
			.on_enter( [this] { m_log += "+1_2_2"; } )
			.on_exit( [this] { m_log += "-1_2_2"; } );

		st_two
			.on_enter( [this] { m_log += "+2"; } )
			.on_exit( [this] { m_log += "-2"; } );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;
		this >>= st_one_1_2_2;
		this >>= so_default_state();

		const std::string expected = "+1+1_1+1_1_1-1_1_1-1_1+1_2+1_2_2"
				"-1_2_2-1_2-1";

		if( expected != m_log )
			throw std::runtime_error( expected + " != " + m_log );

		so_deregister_agent_coop_normally();
	}

private :
	std::string m_log;
};

class a_substates_of_two_state_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };

	state_t st_one_1_1{ initial_substate_of{ st_one } };

	state_t st_one_1_1_1{ initial_substate_of{ st_one_1_1 } };
	state_t st_one_1_1_2{ substate_of{ st_one_1_1 } };

	state_t st_one_1_2{ substate_of{ st_one } };

	state_t st_one_1_2_1{ initial_substate_of{ st_one_1_2 } };
	state_t st_one_1_2_2{ substate_of{ st_one_1_2 } };

	state_t st_two_1_1{ initial_substate_of{ st_two } };

	state_t st_two_1_1_1{ initial_substate_of{ st_two_1_1 } };
	state_t st_two_1_1_2{ substate_of{ st_two_1_1 } };

	state_t st_two_1_2{ substate_of{ st_two } };

	state_t st_two_1_2_1{ initial_substate_of{ st_two_1_2 } };
	state_t st_two_1_2_2{ substate_of{ st_two_1_2 } };


public :
	a_substates_of_two_state_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		st_one
			.on_enter( [this] { m_log += "+1"; } )
			.on_exit( [this] { m_log += "-1"; } );

		st_one_1_1
			.on_enter( [this] { m_log += "+1_1"; } )
			.on_exit( [this] { m_log += "-1_1"; } );

		st_one_1_1_1 
			.on_enter( [this] { m_log += "+1_1_1"; } )
			.on_exit( [this] { m_log += "-1_1_1"; } );

		st_one_1_1_2 
			.on_enter( [this] { m_log += "+1_1_2"; } )
			.on_exit( [this] { m_log += "-1_1_2"; } );

		st_one_1_2
			.on_enter( [this] { m_log += "+1_2"; } )
			.on_exit( [this] { m_log += "-1_2"; } );

		st_one_1_2_1 
			.on_enter( [this] { m_log += "+1_2_1"; } )
			.on_exit( [this] { m_log += "-1_2_1"; } );

		st_one_1_2_2 
			.on_enter( [this] { m_log += "+1_2_2"; } )
			.on_exit( [this] { m_log += "-1_2_2"; } );

		st_two
			.on_enter( [this] { m_log += "+2"; } )
			.on_exit( [this] { m_log += "-2"; } );

		st_two_1_1
			.on_enter( [this] { m_log += "+2_1"; } )
			.on_exit( [this] { m_log += "-2_1"; } );

		st_two_1_1_1 
			.on_enter( [this] { m_log += "+2_1_1"; } )
			.on_exit( [this] { m_log += "-2_1_1"; } );

		st_two_1_1_2 
			.on_enter( [this] { m_log += "+2_1_2"; } )
			.on_exit( [this] { m_log += "-2_1_2"; } );

		st_two_1_2
			.on_enter( [this] { m_log += "+2_2"; } )
			.on_exit( [this] { m_log += "-2_2"; } );

		st_two_1_2_1 
			.on_enter( [this] { m_log += "+2_2_1"; } )
			.on_exit( [this] { m_log += "-2_2_1"; } );

		st_two_1_2_2 
			.on_enter( [this] { m_log += "+2_2_2"; } )
			.on_exit( [this] { m_log += "-2_2_2"; } );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;
		this >>= st_two_1_2_2;
		this >>= so_default_state();

		const std::string expected = "+1+1_1+1_1_1-1_1_1-1_1-1+2+2_2+2_2_2"
				"-2_2_2-2_2-2";

		if( expected != m_log )
			throw std::runtime_error( expected + " != " + m_log );

		so_deregister_agent_coop_normally();
	}

private :
	std::string m_log;
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
								coop.make_agent< a_only_top_level_states_t >();
							} );

						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_substates_of_one_state_t >();
							} );

						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_substates_of_two_state_t >();
							} );
					}/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			20,
			"simple test for on_enter and on_exit handlers" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


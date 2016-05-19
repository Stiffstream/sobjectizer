/*
 * A test for on_exit handlers on environment shutdown.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

struct complete {};

class a_only_top_level_states_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };
	state_t st_three{ this, "three" };

public :
	a_only_top_level_states_t(
		context_t ctx,
		std::string & log,
		so_5::mchain_t notify_ch )
		:	so_5::agent_t( ctx )
		,	m_log( log )
		,	m_notify_ch( std::move(notify_ch) )
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
		this >>= st_two;
		so_5::send< complete >( m_notify_ch );
	}

private :
	std::string & m_log;
	const so_5::mchain_t m_notify_ch;
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
	a_substates_of_one_state_t(
		context_t ctx,
		std::string & log,
		so_5::mchain_t notify_ch )
		:	so_5::agent_t( ctx )
		,	m_log( log )
		,	m_notify_ch( std::move(notify_ch) )
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
		this >>= st_one_1_2_2;
		so_5::send< complete >( m_notify_ch );
	}

private :
	std::string & m_log;

	const so_5::mchain_t m_notify_ch;
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				std::string log1;
				std::string log2;
				so_5::launch( [&]( so_5::environment_t & env ) {
						auto notify_ch = env.create_mchain(
								so_5::make_unlimited_mchain_params() );

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
								coop.make_agent< a_only_top_level_states_t >(
										std::ref(log1), notify_ch );
							} );

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
								coop.make_agent< a_substates_of_one_state_t >(
										std::ref(log2), notify_ch );
							} );

						receive( from( notify_ch ).handle_n( 2 ), []( complete ){} );
						env.stop();

					}/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}*/ );

				const std::string expected1 = "+2-2";
				const std::string expected2 = "+1+1_2+1_2_2-1_2_2-1_2-1";
				ensure( expected1 == log1,
						"unexpected value of log1: " + log1 + ", expected: " + expected1 );
				ensure( expected2 == log2,
						"unexpected value of log2: " + log2 + ", expected: " + expected2 );
			},
			20,
			"test for calling on_exit on environment shutdown" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


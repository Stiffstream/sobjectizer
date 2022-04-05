/*
 * A very simple test case for checking transfer_to_state.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

class a_transfer_message_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };
	state_t st_three{ this, "three" };

	struct message { int m_value; };
	struct finish : public so_5::signal_t {};

public :
	a_transfer_message_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		so_default_state()
			.event( &a_transfer_message_t::evt_finish );

		st_one
			.on_enter( [this] { m_log += "+1"; } )
			.on_exit( [this] { m_log += "-1"; } )
			.transfer_to_state< message >( st_two );

		st_two
			.on_enter( [this] { m_log += "+2"; } )
			.on_exit( [this] { m_log += "-2"; } )
			.transfer_to_state< message >( st_three );

		st_three
			.on_enter( [this] { m_log += "+3"; } )
			.on_exit( [this] { m_log += "-3"; } )
			.event( [this]( const message & msg ) {
					m_log += "{m:" + std::to_string( msg.m_value ) + "}";
					so_default_state().activate();
					so_5::send< finish >( *this );
				} );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;
		so_5::send< message >( *this, 42 );
	}

private :
	std::string m_log;

	void
	evt_finish(mhood_t< finish >)
	{
		const std::string expected = "+1-1+2-2+3{m:42}-3";
		if( expected != m_log )
			throw std::runtime_error( expected + " != " + m_log );

		so_deregister_agent_coop_normally();
	}
};

class a_transfer_signal_t final : public so_5::agent_t
{
	state_t st_one{ this, "one" };
	state_t st_two{ this, "two" };
	state_t st_three{ this, "three" };

	struct signal : public so_5::signal_t {};
	struct finish : public so_5::signal_t {};

public :
	a_transfer_signal_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		so_default_state()
			.event( &a_transfer_signal_t::evt_finish );

		st_one
			.on_enter( [this] { m_log += "+1"; } )
			.on_exit( [this] { m_log += "-1"; } )
			.transfer_to_state< signal >( st_two );

		st_two
			.on_enter( [this] { m_log += "+2"; } )
			.on_exit( [this] { m_log += "-2"; } )
			.transfer_to_state< signal >( st_three );

		st_three
			.on_enter( [this] { m_log += "+3"; } )
			.on_exit( [this] { m_log += "-3"; } )
			.event( [this](mhood_t< signal >) {
					m_log += "{s}";
					so_default_state().activate();
					so_5::send< finish >( *this );
				} );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;

		so_5::send< signal >( *this );
	}

private :
	std::string m_log;

	void
	evt_finish(mhood_t< finish >)
	{
		const std::string expected = "+1-1+2-2+3{s}-3";
		if( expected != m_log )
			throw std::runtime_error( expected + " != " + m_log );

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
								coop.make_agent< a_transfer_message_t >();
							} );
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_transfer_signal_t >();
							} );
					}/*,
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					}*/ );
			},
			20,
			"simple test for transfer_to_state" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


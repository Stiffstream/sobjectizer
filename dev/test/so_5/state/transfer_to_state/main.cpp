/*
 * A very simple test case for checking transfer_to_state.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

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
			.event< finish >( &a_transfer_message_t::evt_finish );

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
					this >>= so_default_state();
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
	evt_finish()
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
			.event< finish >( &a_transfer_signal_t::evt_finish );

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
			.event< signal >( [this]() -> int {
					m_log += "{s}";
					this >>= so_default_state();
					so_5::send< finish >( *this );
					return 42;
				} );
	}

	virtual void
	so_evt_start() override
	{
		this >>= st_one;

		// Service request must be initiated from different working thread.
		so_5::introduce_child_coop( *this,
			so_5::disp::one_thread::create_private_disp(
				so_environment() )->binder(),
			[this]( so_5::coop_t & coop ) {
				coop.define_agent().on_start( [this] {
					const int r = so_5::request_value< int, signal >(
							*this, so_5::infinite_wait );
					if( 42 != r )
						throw std::runtime_error( "unexpected result: " +
								std::to_string( r ) );
				} );
			} );
	}

private :
	std::string m_log;

	void
	evt_finish()
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


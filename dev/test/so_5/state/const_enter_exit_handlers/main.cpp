/*
 * A test for constant on_exit/on_enter handlers.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_supervisor_t final : public so_5::agent_t
{
	state_t first_state_entered{ this, "first_state_entered" };

public :
	struct first_state_enter : public so_5::signal_t {};
	struct first_state_exit : public so_5::signal_t {};

	a_supervisor_t( context_t ctx )
		:	so_5::agent_t(std::move(ctx))
	{
		so_default_state().event( &a_supervisor_t::on_enter );
		first_state_entered.event( &a_supervisor_t::on_exit );
	}

private :
	void
	on_enter(mhood_t<first_state_enter>)
	{
		this >>= first_state_entered;
	}

	void
	on_exit(mhood_t<first_state_exit>)
	{
		so_deregister_agent_coop_normally();
	}
};

class a_test_t final : public so_5::agent_t
{
	state_t first_state{ this, "first" };

	const so_5::mbox_t m_supervisor;

public :
	a_test_t( context_t ctx, so_5::mbox_t supervisor )
		:	so_5::agent_t{ ctx }
		,	m_supervisor{ std::move(supervisor) }
	{
		first_state.on_enter( &a_test_t::on_enter );
		first_state.on_exit( &a_test_t::on_exit );
	}

	virtual void
	so_evt_start() override
	{
		this >>= first_state;
		this >>= so_default_state();
	}

private :
	void on_enter() const
	{
		so_5::send<a_supervisor_t::first_state_enter>( m_supervisor );
	}

	void on_exit() const
	{
		so_5::send<a_supervisor_t::first_state_exit>( m_supervisor );
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
								auto supervisor = coop.make_agent< a_supervisor_t >();
								coop.make_agent< a_test_t >( supervisor->so_direct_mbox() );
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


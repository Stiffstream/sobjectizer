/*
 * Test for checking state_switch_guard.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

class a_test_t final : public so_5::agent_t
{
	struct sig_1 : public so_5::signal_t {};
	struct finish : public so_5::signal_t {};

	state_t st_one{ this };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		// This approach can lead to infinite loop.
		st_one.on_enter( [this] {
				bool exception_thrown = false;
				try {
					this >>= so_default_state();
				}
				catch( const so_5::exception_t & x ) {
					if( so_5::rc_another_state_switch_in_progress == x.error_code() )
						exception_thrown = true;
				}

				ensure_or_die( exception_thrown, "an exception with error_code "
						"rc_another_state_switch_in_progress must be thrown" );

				so_5::send< finish >( *this );
			} )
			.event< finish >( [this]{ so_deregister_agent_coop_normally(); } );

		so_default_state().event< sig_1 >( [this] {
				this >>= st_one;
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< sig_1 >( *this );
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
			"simple test for state_switch_guard" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


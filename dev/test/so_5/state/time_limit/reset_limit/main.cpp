/*
 * A test case for reseting time limit for a state.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t final : public so_5::agent_t
{
	state_t first{ this, "first" };
	state_t second{ this, "second" };

	struct sig_1 : public so_5::signal_t {};
	struct sig_2 : public so_5::signal_t {};

	struct finish : public so_5::signal_t {};

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{
		first
			.on_enter( []{ std::cout << "first on_enter" << std::endl; } )
			.on_exit( [this] {
				std::cout << "first on_exit" << std::endl;
				so_5::send< finish >( *this );
			} )
			.time_limit( std::chrono::milliseconds{100}, second )
			.event< sig_1 >( [this] {
				std::cout << "first sig_1, dropping time_limit" << std::endl;
				first.drop_time_limit();
				so_5::send_delayed< sig_2 >( *this, std::chrono::milliseconds{200} );
			} )
			.event< sig_2 >( [this] {
				std::cout << "first sig_2, resetting time_limit" << std::endl;
				first.time_limit( std::chrono::milliseconds{50}, second );
			} );

		second
			.on_enter( []{ std::cout << "second on_enter" << std::endl; } )
			.event< finish >( [this] {
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		this >>= first;
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
					},
					[]( so_5::environment_params_t & params ) {
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			20,
			"test for resetting state's time_limit" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


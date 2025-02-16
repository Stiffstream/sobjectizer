/*
 * Test for checking time_limit for parent and child states.
 */

#include <iostream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

namespace test
{

class a_test_t final : public so_5::agent_t
{
	state_t top_one{ this, "top_one" };
	state_t child_one{ initial_substate_of{ top_one }, "child_one" };
	state_t child_two{ substate_of{ top_one }, "child_two" };

	state_t top_two{ this, "top_two" };

	state_t top_three{ this, "top_three" };

	struct make_pause final : public so_5::signal_t {};
	struct finish final : public so_5::signal_t {};

	std::chrono::steady_clock::time_point m_top_two_enter_at;
	static const std::chrono::steady_clock::duration top_two_time_limit;

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t{ ctx }
	{}

	void
	so_define_agent() override
	{
		top_one
			.on_enter( [this]{
					std::cout << "top_one on_enter" << std::endl;
					so_5::send< make_pause >( *this );
				} )
			.on_exit( []{ std::cout << "top_one on_exit" << std::endl; } )
			.time_limit( std::chrono::milliseconds{ 200 }, top_two )
			.event( [this]( mhood_t<make_pause> ) {
					// Suspend the agent for some time.
					std::this_thread::sleep_for( std::chrono::milliseconds{ 250 } );
				} )
			;

		child_one
			.on_enter( []{ std::cout << "child_one on_enter" << std::endl; } )
			.on_exit( []{ std::cout << "child_one on_exit" << std::endl; } )
			.time_limit( std::chrono::milliseconds{ 100 }, child_two )
			;
		child_two
			.on_enter(
					[]{
						std::cout << "child_two on_enter" << std::endl;
						throw std::runtime_error{ "unexpected enter into child_two state" };
					} )
			.on_exit( []{ std::cout << "child_two on_exit" << std::endl; } )
			;

		top_two
			.on_enter( [this]{
					std::cout << "top_two on_enter" << std::endl;
					m_top_two_enter_at = std::chrono::steady_clock::now();
				} )
			.on_exit( [this] {
					const auto now = std::chrono::steady_clock::now();
					std::cout << "top_two on_exit" << std::endl;
					if( m_top_two_enter_at + top_two_time_limit > now )
					{
						const auto elapsed = now - m_top_two_enter_at;
						using namespace std::chrono;
						const std::string message =
								"timeout for top_two is too small: " +
								std::to_string(
										duration_cast< microseconds >( elapsed ).count() ) +
								"us";
						std::cout << message << std::endl;

						throw std::runtime_error{ message };
					}
				} )
			.time_limit( top_two_time_limit, top_three )
			;

		top_three
			.on_enter( [this]{
					std::cout << "top_three on_enter" << std::endl;
					so_5::send< finish >( *this );
				} )
			.event( [this](mhood_t< finish >) {
					so_deregister_agent_coop_normally();
				} )
			;
	}

	void
	so_evt_start() override
	{
		this >>= top_one;
	}
};

const std::chrono::steady_clock::duration a_test_t::top_two_time_limit =
		std::chrono::milliseconds{ 30 };

} /* namespace test */

using namespace test;

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
						// More precise timer than timer_wheel.
						params.timer_thread( so_5::timer_list_factory() );
#if 0
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
#endif
					} );
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


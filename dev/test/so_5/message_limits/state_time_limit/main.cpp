/*
 * A test for using message limits and time limit for a state.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <so_5/impl/internal_env_iface.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

struct msg_timeout : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	state_t st_wait_timeout{ this, "wait_timeout" };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx + limit_then_abort< msg_timeout >( 1 ) )
	{}

	void
	so_define_agent() override
	{
		st_wait_timeout
			// Message from the standard direct mbox.
			.event( [this](mhood_t< msg_timeout >) {
					so_deregister_agent_coop_normally();
				} )
			.time_limit( std::chrono::seconds{ 2 }, so_default_state() )
			;

		this >>= st_wait_timeout;
	}

	void
	so_evt_start() override
	{
		so_5::send_delayed< msg_timeout >( *this, std::chrono::milliseconds{50} );
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
						env.register_agent_as_coop( env.make_agent< a_test_t >() );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


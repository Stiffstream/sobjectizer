/*
 * A test for any_unspecified_message and state.time_limit.
 * (A limitless mpsc mbox has to be created for controlling time
 * limit for a state).
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

struct msg_start : public so_5::signal_t {};
struct msg_finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	state_t st_first{ this, "first" };
	state_t st_second{ this, "second" };
	state_t st_third{ this, "third" };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx
				+ limit_then_abort< msg_start >( 1 )
				+ limit_then_abort< msg_finish >( 1 )
				+ limit_then_abort< any_unspecified_message >( 2 ) )
	{}

	void
	so_define_agent() override
	{
		so_default_state()
			.event( &a_test_t::evt_start )
			.event( &a_test_t::evt_finish )
			;

		st_first
			.time_limit( std::chrono::milliseconds{25}, so_default_state() )
			;

		st_second
			.time_limit( std::chrono::milliseconds{25}, so_default_state() )
			;

		st_third
			.time_limit( std::chrono::milliseconds{25}, so_default_state() )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_start >( *this );
	}

private:
	void
	evt_start( mhood_t<msg_start> )
	{
		st_first.activate();
		std::this_thread::sleep_for( std::chrono::milliseconds{50} );

		st_second.activate();
		std::this_thread::sleep_for( std::chrono::milliseconds{50} );

		st_third.activate();
		std::this_thread::sleep_for( std::chrono::milliseconds{50} );

		so_default_state().activate();

		so_5::send< msg_finish >( *this );
	}

	void
	evt_finish( mhood_t<msg_finish> )
	{
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


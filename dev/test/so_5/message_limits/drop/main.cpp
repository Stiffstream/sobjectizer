/*
 * A simple test for message limits (dropping the message).
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_one : public so_5::signal_t {};
struct msg_two : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_drop< msg_one >( 2 )
				+ limit_then_drop< msg_two >( 1000 ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event< msg_one >( [&]{ ++m_received; } )
			.event< msg_two >( [&]{
				if( 2 != m_received )
					throw std::runtime_error( "unexpected count of "
							"received msg_one instances: " +
							std::to_string( m_received ) );

				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< msg_one >( *this );
		so_5::send_to_agent< msg_one >( *this );
		so_5::send_to_agent< msg_one >( *this );
		so_5::send_to_agent< msg_one >( *this );
		so_5::send_to_agent< msg_one >( *this );
		so_5::send_to_agent< msg_one >( *this );

		so_5::send_to_agent< msg_two >( *this );
	}

private :
	unsigned int m_received = 0;
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_test_t >() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init );
			},
			20,
			"simple message drop test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


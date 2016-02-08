/*
 * A simple test for message limits (exception during attempt to
 * subscribe to the message without limit predefined).
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
struct msg_three : public so_5::signal_t {};

class a_with_limits_t : public so_5::agent_t
{
public :
	a_with_limits_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_drop< msg_one >( 2 )
				+ limit_then_drop< msg_two >( 1000 ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_one >( []{} );

		so_default_state().event< msg_two >( []{} );

		bool exception_thrown = false;
		try
		{
			so_default_state().event< msg_three >( []{} );
		}
		catch( const so_5::exception_t & ex )
		{
			if( so_5::rc_message_has_no_limit_defined == ex.error_code() )
				exception_thrown = true;
			else
				throw std::runtime_error( "unexpected exception with code: " +
						std::to_string( ex.error_code() ) );
		}

		if( !exception_thrown )
			throw std::runtime_error( "exception expected during subscription "
					"to message without a limit" );
	}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

class a_without_limits_t : public so_5::agent_t
{
public :
	a_without_limits_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event< msg_one >( []{} );

		so_default_state().event< msg_two >( []{} );

		so_default_state().event< msg_three >( []{} );
	}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

void
case_with_message_limits( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_with_limits_t >() );
}

void
case_without_message_limits( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_without_limits_t >() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( case_with_message_limits );
				so_5::launch( case_without_message_limits );
			},
			20,
			"subscription without limit" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


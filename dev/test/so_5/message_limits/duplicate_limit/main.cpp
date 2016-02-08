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

class a_wrong_agent_t : public so_5::agent_t
{
public :
	a_wrong_agent_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_drop< msg_one >( 2 )
				+ limit_then_drop< msg_two >( 1000 )
				+ limit_then_redirect< msg_one >( 3,
					[this] { return so_direct_mbox(); } ) )
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

class a_normal_agent_t : public so_5::agent_t
{
public :
	a_normal_agent_t(
		so_5::environment_t & env )
		:	so_5::agent_t( env
				+ limit_then_drop< msg_one >( 1 )
				+ limit_then_drop< msg_two >( 1 ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event< msg_two >( [&]{
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< msg_one >( *this );

		so_5::send_to_agent< msg_two >( *this );
	}
};

void
init( so_5::environment_t & env )
{
	auto coop = env.create_coop( so_5::autoname );

	try
	{
		coop->make_agent< a_wrong_agent_t >();
		throw std::runtime_error( "Exception from SObjectizer "
				"about duplicate limit expected" );
	}
	catch( const so_5::exception_t & x )
	{
		if( so_5::rc_several_limits_for_one_message_type != x.error_code() )
			throw;
	}

	coop->make_agent< a_normal_agent_t >();

	env.register_coop( std::move( coop ) );
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
			"duplicate message limit test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


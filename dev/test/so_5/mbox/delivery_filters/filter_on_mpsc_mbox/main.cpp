/*
 * A test for defining delivery filter for MPSC-mboxes.
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

struct data { int m_key; };

struct finish : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		try
		{
			so_set_delivery_filter( so_direct_mbox(),
				[]( const data & msg ) {
					return 1 == msg.m_key;
				} );
		}
		catch( const so_5::exception_t & x )
		{
			if( so_5::rc_delivery_filter_cannot_be_used_on_mpsc_mbox !=
					x.error_code() )
				throw std::runtime_error( "unexpected exception code: " +
						std::to_string( x.error_code() ) );
		}

		so_default_state()
			.event< finish >( [this] {
				so_deregister_agent_coop_normally();
			} );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send_to_agent< finish >( *this );
	}
};

class a_test_with_limits_t : public a_test_t
{
public :
	a_test_with_limits_t( context_t ctx )
		:	a_test_t( ctx
				+ limit_then_drop< data >( 1 )
				+ limit_then_abort< finish >( 1 ) )
	{}
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_test_t >() );
	env.register_agent_as_coop( so_5::autoname,
			env.make_agent< a_test_with_limits_t >() );
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
			"delivery filter for MPSC-mboxes" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


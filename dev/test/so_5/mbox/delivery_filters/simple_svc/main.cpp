/*
 * A simple test for delivery filters and service requests.
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

class a_provider_t : public so_5::agent_t
{
public :
	a_provider_t( context_t ctx, so_5::mbox_t svc_mbox )
		:	so_5::agent_t( ctx )
		,	m_svc_mbox( std::move( svc_mbox ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_set_delivery_filter( m_svc_mbox, []( const data & msg ) {
				return 1 == msg.m_key;
			} );

		so_subscribe( m_svc_mbox ).event( []( const data & msg ) {
				return 1 + msg.m_key;
			} );
	}

private :
	const so_5::mbox_t m_svc_mbox;
};

class a_consumer_t : public so_5::agent_t
{
public :
	a_consumer_t( context_t ctx, so_5::mbox_t svc_mbox )
		:	so_5::agent_t( ctx )
		,	m_svc_mbox( std::move( svc_mbox ) )
	{}

	virtual void
	so_evt_start() override
	{
		auto proxy = m_svc_mbox->get_one< int >().wait_forever();
		try {
			proxy.make_sync_get< data >( 0 );
			throw std::runtime_error( "unexpected result for value 0" );
		}
		catch( const so_5::exception_t & x )
		{
			if( so_5::rc_no_svc_handlers != x.error_code() )
				throw std::runtime_error( "unexpected error_code: " +
						std::to_string( x.error_code() ) );
		}

		auto r = proxy.make_sync_get< data >( 1 );
		if( 2 != r )
			throw std::runtime_error( "unexpected result for value 1: " +
					std::to_string( r ) );

		so_deregister_agent_coop_normally();
	}

private :
	const so_5::mbox_t m_svc_mbox;
};

void
init( so_5::environment_t & env )
{
	env.introduce_coop(
			so_5::disp::active_obj::create_private_disp( env )->binder(),
			[&]( so_5::coop_t & coop ) {
				const auto mbox = env.create_mbox();

				coop.make_agent< a_provider_t >( mbox );
				coop.make_agent< a_consumer_t >( mbox );
			} );
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
			"simple delivery filter for service_request test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


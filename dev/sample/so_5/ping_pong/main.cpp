#include <iostream>
#include <set>

#include <cstdio>
#include <cstring>

#include <so_5/api/h/api.hpp>
#include <so_5/rt/h/rt.hpp>

#include <so_5/disp/active_obj/h/pub.hpp>

struct	cfg_t
{
	unsigned int	m_request_count;

	bool	m_active_objects;

	cfg_t()
		:	m_request_count( 1000 )
		,	m_active_objects( false )
		{}
};

cfg_t
try_parse_cmdline(
	int argc,
	char ** argv )
	{
		if( 1 == argc )
			{
				std::cout << "usage:\n"
						"sample.so_5.ping_pong <options>\n"
						"\noptions:\n"
						"-a, --active-objects agents should be active objects\n"
						"-r, --requests       count of requests to send\n"
						<< std::endl;

				throw std::runtime_error( "No command-line errors" );
			}

		auto is_arg = []( const char * value,
				const char * v1,
				const char * v2 )
				{
					return 0 == std::strcmp( value, v1 ) ||
							0 == std::strcmp( value, v2 );
				};

		cfg_t result;

		char ** current = argv + 1;
		char ** last = argv + argc;

		while( current != last )
			{
				if( is_arg( *current, "-a", "--active-objects" ) )
					{
						result.m_active_objects = true;
					}
				else if( is_arg( *current, "-r", "--requests" ) )
					{
						++current;
						if( current == last )
							throw std::runtime_error( "-r requires argument" );

						result.m_request_count = std::atoi( *current );
					}
				else
					{
						throw std::runtime_error(
							std::string( "unknown argument: " ) + *current );
					}
				++current;
			}

		return result;
	}

void
show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "active objects: " << ( cfg.m_active_objects ? "yes" : "no" )
			<< ", requests: " << cfg.m_request_count
			<< std::endl;
	}

void
run_sample(
	const cfg_t & cfg )
	{
		// This variable will be a part of pinger agent's state.
		unsigned int pings_left = cfg.m_request_count;

		so_5::api::run_so_environment(
			[&pings_left, &cfg]( so_5::rt::environment_t & env )
			{
				// Types of signals for the agents.
				struct msg_ping : public so_5::rt::signal_t {};
				struct msg_pong : public so_5::rt::signal_t {};

				auto mbox = env.create_local_mbox();

				auto coop = env.create_coop( "ping_pong",
					// Agents will be active or passive.
					// It depends on sample arguments.
					cfg.m_active_objects ?
						so_5::disp::active_obj::create_disp_binder( "active_obj" ) :
						so_5::rt::create_default_disp_binder() );

				// Pinger agent.
				coop->define_agent()
					.on_start( [mbox]() { mbox->deliver_signal< msg_ping >(); } )
					.event( mbox, so_5::signal< msg_pong >,
						[&pings_left, &env, mbox]()
						{
							if( pings_left ) --pings_left;
							if( pings_left )
								mbox->deliver_signal< msg_ping >();
							else
								env.stop();
						} );

				// Ponger agent.
				coop->define_agent()
					.event( mbox, so_5::signal< msg_ping >,
						[mbox]() { mbox->deliver_signal< msg_pong >(); } );

				env.register_coop( std::move( coop ) );
			},
			[&cfg]( so_5::rt::environment_params_t & p )
			{
				if( cfg.m_active_objects )
					// Special dispatcher is necessary for agents.
					p.add_named_dispatcher( "active_obj",
						so_5::disp::active_obj::create_disp() );
			} );
	}

int
main( int argc, char ** argv )
{
	try
	{
		cfg_t cfg = try_parse_cmdline( argc, argv );
		show_cfg( cfg );

		run_sample( cfg );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}


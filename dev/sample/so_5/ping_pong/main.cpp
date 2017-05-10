#include <iostream>
#include <set>

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <so_5/all.hpp>

struct	cfg_t
{
	unsigned int	m_request_count;

	bool	m_active_objects;

	cfg_t()
		:	m_request_count( 1000 )
		,	m_active_objects( false )
		{}
};

cfg_t try_parse_cmdline(
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

						result.m_request_count = static_cast< unsigned int >(
								std::atoi( *current ) );
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

void show_cfg(
	const cfg_t & cfg )
	{
		std::cout << "Configuration: "
			<< "active objects: " << ( cfg.m_active_objects ? "yes" : "no" )
			<< ", requests: " << cfg.m_request_count
			<< std::endl;
	}

void run_sample(
	const cfg_t & cfg )
	{
		// This variable will be a part of pinger agent's state.
		unsigned int pings_left = cfg.m_request_count;

		so_5::launch(
			[&pings_left, &cfg]( so_5::environment_t & env )
			{
				// Types of signals for the agents.
				struct msg_ping : public so_5::signal_t {};
				struct msg_pong : public so_5::signal_t {};

				env.introduce_coop(
					// Agents will be active or passive.
					// It depends on sample arguments.
					cfg.m_active_objects ?
						so_5::disp::active_obj::create_private_disp( env )->binder() :
						so_5::make_default_disp_binder( env ),
						[&]( so_5::coop_t & coop )
						{
							auto mbox = env.create_mbox();

							// Pinger agent.
							coop.define_agent()
								.on_start( [mbox]() { so_5::send< msg_ping >( mbox ); } )
								.event< msg_pong >( mbox,
									[&pings_left, &env, mbox]()
									{
										if( pings_left ) --pings_left;
										if( pings_left )
											so_5::send< msg_ping >( mbox );
										else
											env.stop();
									} );

							// Ponger agent.
							coop.define_agent()
								.event< msg_ping >( mbox,
									[mbox]() { so_5::send< msg_pong >( mbox ); } );
						} );
			} );
	}

int main( int argc, char ** argv )
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


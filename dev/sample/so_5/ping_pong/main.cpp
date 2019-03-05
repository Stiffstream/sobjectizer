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
		// Types of signals for the agents.
		struct msg_ping final : public so_5::signal_t {};
		struct msg_pong final : public so_5::signal_t {};

		// Type of pinger agent.
		class pinger_t final : public so_5::agent_t
			{
				const so_5::mbox_t m_mbox;
				unsigned int m_pings_left;
			public :
				pinger_t(
					context_t ctx,
					so_5::mbox_t mbox,
					unsigned int pings_left )
					:	so_5::agent_t{ std::move(ctx) }
					,	m_mbox{ std::move(mbox) }
					,	m_pings_left{ pings_left }
					{
						so_subscribe( m_mbox ).event(
							[this]( mhood_t<msg_pong> ) {
								if( m_pings_left ) --m_pings_left;
								if( m_pings_left )
									so_5::send< msg_ping >( m_mbox );
								else
									so_environment().stop();
							} );
					}

				void so_evt_start() override
					{
						so_5::send< msg_ping >( m_mbox );
					}
			};

		// Type of ponger agent.
		class ponger_t final : public so_5::agent_t
			{
			public :
				ponger_t( context_t ctx, const so_5::mbox_t & mbox )
					:	so_5::agent_t{ std::move(ctx) }
					{
						so_subscribe( mbox ).event(
							[mbox]( mhood_t<msg_ping> ) {
								so_5::send< msg_pong >( mbox );
							} );
					}
			};

		so_5::launch(
			[&cfg]( so_5::environment_t & env )
			{
				env.introduce_coop(
					// Agents will be active or passive.
					// It depends on sample arguments.
					cfg.m_active_objects ?
						so_5::disp::active_obj::make_dispatcher( env ).binder() :
						so_5::make_default_disp_binder( env ),
						[&]( so_5::coop_t & coop )
						{
							auto mbox = env.create_mbox();

							// Pinger agent.
							coop.make_agent< pinger_t >( mbox, cfg.m_request_count );
							// Ponger agent.
							coop.make_agent< ponger_t >( std::cref(mbox) );
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


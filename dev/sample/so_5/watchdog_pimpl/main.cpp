#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Watchdog public interface.
#include "watchdog.hpp"

using namespace std;
using namespace std::chrono;

// Sample configuration.
struct cfg_t
{
	// Virtual operation duration.
	steady_clock::duration m_op_duration = milliseconds( 2015 );

	// Operation watchdog timeout.
	steady_clock::duration m_op_timeout = milliseconds( 2500 );
};

cfg_t
try_parse_cmdline( int argc, char ** argv )
{
	auto is_arg =
		[]( const char * value, const char * v1, const char * v2 ){
			return 0 == strcmp( value, v1 ) ||
					0 == strcmp( value, v2 );
		};

	cfg_t result;

	char ** current = argv + 1;
	char ** last = argv + argc;

	while( current != last )
	{
		if( is_arg( *current, "-o", "--operation-duration" ) )
		{
			++current;
			if( current == last )
				throw runtime_error( "-o requires argument" );

			result.m_op_duration = milliseconds( atoi( *current ) );
		}
		else if( is_arg( *current, "-t", "--operation-timeout" ) )
		{
			++current;
			if( current == last )
				throw runtime_error( "-t requires argument" );

			result.m_op_timeout = milliseconds( atoi( *current ) );
		}
		else
		{
			cout << "usage:\n"
					"sample.so_5.watch_dog_pimpl <options>\n"
					"\noptions:\n"
					"-o, --operation-duration    Operation duration.\n"
					"-t, --operation-timeout     Operation timeout.\n"
					<< endl;

			throw runtime_error(
				string( "unknown argument: " ) + *current );
		}

		++current;
	}

	return result;
}

void
show_cfg(
	const cfg_t & cfg )
{
	auto ms = []( steady_clock::duration d ) {
			return duration_cast< milliseconds >( d ).count();
		};
	cout << "Configuration:\n"
		"operation duration: " << ms( cfg.m_op_duration ) << " ms.\n"
		"operation timeout:  " << ms( cfg.m_op_timeout ) << " ms.\n"
		<< endl;
}

void
init( cfg_t cfg, so_5::rt::environment_t & env )
{
	// Creating a cooperation.
	auto coop = env.create_coop( "coop" );

	auto watchdog_agent = coop->add_agent(
			new a_watchdog_t( env ),
			// Watchdog must run in a separate thread.
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

	auto watchdog = watchdog_agent->create_watchdog();

	// Agent which imitates some long-running application logic.
	coop->define_agent()
		.on_start(
			[&env, watchdog, cfg ]()
			{
				cout << "Start long operation..." << endl;

				{
					// Perfom watched operation.
					operation_watchdog_t op(
						watchdog,
						"Long operation",
						cfg.m_op_timeout );

					this_thread::sleep_for( cfg.m_op_duration );
				}

				cout << "Finish long operation..." << endl;

				// Shutting down SObjectizer.
				env.stop();
			} );

	// Registering the cooperation.
	env.register_coop( move( coop ) );
}

int
main( int argc, char ** argv )
{
	using namespace std::placeholders;

	try
	{
		auto cfg = try_parse_cmdline( argc, argv );

		show_cfg( cfg );

		// Create SO Environment objects and run SO Run-Time inside it.
		so_5::launch(
			// SO Environment initialization routine.
			bind( &init, cfg, _1 ),
			// SO Environment tuning routine.
			[]( so_5::rt::environment_params_t & p )
			{
				p.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );
			});
	}
	catch( const exception & x )
	{
		cerr << "Exception: " << x.what() << endl;
	}
}


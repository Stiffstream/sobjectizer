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

void
init( so_5::rt::environment_t & env )
{
	// Creating a cooperation.
	// Every agent should work on its own thread.
	auto coop = env.create_coop(
			so_5::autoname,
			so_5::disp::active_obj::create_disp_binder( "active_obj" ) );

	auto watchdog = coop->add_agent( new a_watchdog_t( env ) );

	// The first agent which performs some long-running operations.
	coop->define_agent()
		.on_start( [&env, watchdog]
			{
				const string tag = "One";
				const auto timeout = milliseconds( 400 );
				const auto delta = milliseconds( 10 );

				for( int i = 0; i != 100; ++i )
				{
					cout << tag + "(" + to_string(i) + "): started\n" << flush;

					{
						operation_watchdog_t op( watchdog, tag, timeout );
						this_thread::sleep_for( timeout - delta );
					}

					cout << tag + "(" + to_string(i) + "): finished\n" << flush;
				}

				// Shutting down SObjectizer.
				env.stop();
			} );

	// The second agent which performs another long-running operation.
	// This agent must cause abort of application.
	coop->define_agent()
		.on_start( [&env, watchdog]
			{
				const string tag = "Two";
				const auto timeout = milliseconds( 1500 );

				cout << tag + ": started\n" << flush;

				{
					operation_watchdog_t op( watchdog, tag, timeout );
					this_thread::sleep_for( timeout + timeout );
				}

				cout << tag + ": finished\n" << flush;
			} );

	// Registering the cooperation.
	env.register_coop( move( coop ) );
}

int
main()
{
	try
	{
		// Create SO Environment objects and run SO Run-Time inside it.
		so_5::launch(
			// SO Environment initialization routine.
			&init,
			// SO Environment tuning routine.
			[]( so_5::rt::environment_params_t & p )
			{
				// A dispatcher with active objects necessary
				// to run agents on different threads.
				p.add_named_dispatcher(
					"active_obj",
					so_5::disp::active_obj::create_disp() );

				// A precise timer will be useful for this example.
				p.timer_thread( so_5::timer_heap_factory() );
			});
	}
	catch( const exception & x )
	{
		cerr << "Exception: " << x.what() << endl;
	}
}


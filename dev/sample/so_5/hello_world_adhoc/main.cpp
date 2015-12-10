/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

int
main()
{
	using namespace std;
	using namespace so_5;

	try
	{
		so_5::launch( []( environment_t & env )
			{
				// Creating and registering a cooperation.
				env.introduce_coop( [&env]( coop_t & coop ) {
					// Adding agent to the cooperation.
					coop.define_agent()
						// A reaction to start of work in SObjectizer.
						.on_start( [&env] {
								cout << "Hello, world! This is SObjectizer v.5." << endl;

								// Shutting down SObjectizer.
								env.stop();
							} )
						// A reaction to finish of work in SObjectizer.
						.on_finish( [] {
								cout << "Bye! This was SObjectizer v.5." << endl;
							} );
				});
			} );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

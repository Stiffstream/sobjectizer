/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

int
main( int, char ** )
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
				// Creating a cooperation.
				auto coop = env.create_coop( "coop" );

				// Adding agent to the cooperation.
				coop->define_agent()
					// A reaction to start of work in SObjectizer.
					.on_start(
						[&env]() {
							std::cout << "Hello, world! This is SObjectizer v.5."
									<< std::endl;

							// Shutting down SObjectizer.
							env.stop();
						} )
					// A reaction to finish of work in SObjectizer.
					.on_finish(
						[]() {
							std::cout << "Bye! This was SObjectizer v.5."
									<< std::endl;
						} );

				// Registering the cooperation.
				env.register_coop( std::move( coop ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

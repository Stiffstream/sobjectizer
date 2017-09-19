/*
 * A sample of "Hello, World" for simple_not_mtsafe environment infrastructure.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

int main()
{
	using namespace std;
	using namespace so_5;

	so_5::launch( []( environment_t & env )
		{
			// Creating and registering a cooperation.
			env.introduce_coop( []( coop_t & coop ) {
				// Adding agent to the cooperation.
				coop.define_agent()
					// A reaction to start of work in SObjectizer.
					.on_start( []{ cout << "Hello, world!" << endl; } );
			});
		},
		[]( environment_params_t & params ) {
			params.infrastructure_factory(
					so_5::env_infrastructures::simple_not_mtsafe::factory() );
		} );

	return 0;
}


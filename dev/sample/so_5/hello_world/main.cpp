/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of an agent for SObjectizer.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx ) : so_5::agent_t( ctx )
		{}

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override
		{
			std::cout << "Hello, world! This is SObjectizer v.5 ("
					<< SO_5_VERSION << ")" << std::endl;

			// Shutting down SObjectizer.
			so_environment().stop();
		}

		// A reaction to finish of work in SObjectizer.
		virtual void so_evt_finish() override
		{
			std::cout << "Bye! This was SObjectizer v.5." << std::endl;
		}
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering single agent as a cooperation.
				env.register_agent_as_coop( "coop", env.make_agent< a_hello_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

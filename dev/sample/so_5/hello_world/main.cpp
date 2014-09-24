/*
 * A sample of the simpliest agent.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of an agent for SObjectizer.
class a_hello_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_hello_t( so_5::rt::environment_t & env )
			: base_type_t( env )
		{}
		virtual ~a_hello_t()
		{}

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start()
		{
			std::cout << "Hello, world! This is SObjectizer v.5."
				<< std::endl;

			// Shutting down SObjectizer.
			so_environment().stop();
		}

		// A reaction to finish of work in SObjectizer.
		virtual void
		so_evt_finish()
		{
			std::cout << "Bye! This was SObjectizer v.5."
				<< std::endl;
		}
};

int
main( int, char ** )
{
	try
	{
		// Starting SObjectizer.
		so_5::api::run_so_environment(
			// A function for SO Environment initialization.
			[]( so_5::rt::environment_t & env )
			{
				// Creating and registering single agent as a cooperation.
				env.register_agent_as_coop( "coop", new a_hello_t( env ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

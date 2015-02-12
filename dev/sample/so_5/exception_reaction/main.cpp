/*
 * A sample for the exception reaction.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// A class of an agent which will throw an exception.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t( so_5::rt::environment_t & env )
			: so_5::rt::agent_t( env )
		{}
		virtual ~a_hello_t()
		{}

		virtual void
		so_evt_start() override
		{
			throw std::runtime_error( "sample exception" );
		}

		virtual so_5::rt::exception_reaction_t
		so_exception_reaction() const override
		{
			return so_5::rt::shutdown_sobjectizer_on_exception;
		}
};

int
main()
{
	try
	{
		so_5::launch( []( so_5::rt::environment_t & env ) {
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


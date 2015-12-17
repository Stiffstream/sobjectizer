/*
 * A sample of a simple agent which send message to itself.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// An agent class definition.
class a_hello_t : public so_5::agent_t
{
	public:
		a_hello_t( context_t ctx ) : so_5::agent_t( ctx )
		{}

		// Definition of an agent for SObjectizer.
		virtual void so_define_agent() override
		{
			// The hello message subscription.
			so_subscribe_self().event( [this]( const std::string & msg ) {
					std::cout << msg << std::endl;

					// Shutting down SObjectizer.
					so_environment().stop();
				} );
		}

		// A reaction to start of work in SObjectizer.
		virtual void so_evt_start() override
		{
			// Send hello message.
			so_5::send< std::string >( *this, "Hello, world! This is SObjectizer-5.");
		}
};

int main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
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


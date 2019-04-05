/*
 * A sample of a simple agent which send message to itself.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// An agent class definition.
class a_hello_t final : public so_5::agent_t
{
	public:
		using so_5::agent_t::agent_t;

		// Definition of an agent for SObjectizer.
		void so_define_agent() override
		{
			// The hello message subscription.
			so_subscribe_self().event( &a_hello_t::evt_hello );
		}

		// A reaction to start of work in SObjectizer.
		void so_evt_start() override
		{
			// Send hello message.
			so_5::send< std::string >( *this, "Hello, world! This is SObjectizer-5.");
		}

	private :
		// Hello message handler.
		void evt_hello( const std::string & msg )
		{
			std::cout << msg << std::endl;

			// Shutting down SObjectizer.
			so_environment().stop();
		}
};

int main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				env.register_agent_as_coop( env.make_agent< a_hello_t >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


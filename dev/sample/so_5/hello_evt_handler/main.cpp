/*
 * A sample of a simple agent which send message to itself.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Hello message.
struct msg_hello : public so_5::rt::message_t
{
	// Greeting.
	std::string m_message;

	msg_hello( std::string message ) : m_message( std::move( message ) )
	{}
};

// An agent class definition.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t( so_5::rt::environment_t & env )
			: so_5::rt::agent_t( env )
		{}

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent() override
		{
			// The hello message subscription.
			so_subscribe_self().event( &a_hello_t::evt_hello );
		}

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override
		{
			// Send hello message.
			so_5::send_to_agent< msg_hello >( *this,
					"Hello, world! This is SObjectizer-5.");
		}

		// Hello message handler.
		void
		evt_hello( const msg_hello & msg )
		{
			std::cout << msg.m_message << std::endl;

			// Shutting down SObjectizer.
			so_environment().stop();
		}
};

int
main( int, char ** )
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
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


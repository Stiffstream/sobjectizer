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
};

// An agent class definition.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t( so_5::rt::environment_t & env )
			: so_5::rt::agent_t( env )
		{}
		virtual ~a_hello_t()
		{}

		// Definition of an agent for SObjectizer.
		virtual void
		so_define_agent() override;

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start() override;

		// Hello message handler.
		void
		evt_hello( const msg_hello & msg );
};

void
a_hello_t::so_define_agent()
{
	// The hello message subscription.
	so_subscribe_self().event( &a_hello_t::evt_hello );
}

void
a_hello_t::so_evt_start()
{
	std::unique_ptr< msg_hello > msg( new msg_hello );
	msg->m_message = "Hello, world! This is SObjectizer v.5.";

	// Sent hello message.
	so_direct_mbox()->deliver_message( std::move(msg) );
}

void
a_hello_t::evt_hello( const msg_hello & msg )
{
	std::cout << msg.m_message << std::endl;

	// Shutting down SObjectizer.
	so_environment().stop();
}

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


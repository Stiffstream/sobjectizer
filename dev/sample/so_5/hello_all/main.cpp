/*
 * A sample of the message interchange.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

struct msg_hello_to_all : public so_5::rt::message_t
{
	msg_hello_to_all(
		std::string sender,
		const so_5::rt::mbox_ref_t & mbox )
		:	m_sender( std::move( sender ) ),
			m_mbox( mbox )
	{}

	// Sender name.
	const std::string m_sender;
	// Sender Mbox.
	const so_5::rt::mbox_ref_t m_mbox;
};

struct msg_hello_to_you : public so_5::rt::message_t
{
	msg_hello_to_you( std::string sender )
		: m_sender( std::move( sender ) )
	{}

	// Sender name.
	const std::string m_sender;
};


// An agent class.
class a_hello_t : public so_5::rt::agent_t
{
	public:
		a_hello_t(
			so_5::rt::environment_t & env,
			const std::string & agent_name )
			:	so_5::rt::agent_t( env ),
				m_agent_name( agent_name ),
				m_common_mbox( so_environment().create_local_mbox( "common_mbox" ) )
		{}
		virtual ~a_hello_t()
		{}

		// Definition of the agent for SObjectizer.
		virtual void
		so_define_agent();

		// A reaction to start of work in SObjectizer.
		virtual void
		so_evt_start();

		void
		evt_hello_to_all(
			const msg_hello_to_all & evt_data );

		void
		evt_hello_to_you(
			const msg_hello_to_you & evt_data );

	private:
		// Agent name.
		const std::string m_agent_name;

		// Common mbox for all sample agents.
		const so_5::rt::mbox_ref_t m_common_mbox;
};

void
a_hello_t::so_define_agent()
{
	// Message subscription.
	so_subscribe( m_common_mbox )
		.event( &a_hello_t::evt_hello_to_all );

	so_subscribe( so_direct_mbox() )
		.event( &a_hello_t::evt_hello_to_you );
}

void
a_hello_t::so_evt_start()
{
	std::cout << m_agent_name << ".so_evt_start" << std::endl;

	// Send greeting to all agents.
	m_common_mbox->deliver_message(
		new msg_hello_to_all( m_agent_name, so_direct_mbox() ) );
}

void
a_hello_t::evt_hello_to_all(
	const msg_hello_to_all & evt_data )
{
	std::cout << m_agent_name << ".evt_hello_to_all: "
		<< evt_data.m_sender << std::endl;

	// If this agent is not the sender then reply should be sent.
	if( m_agent_name != evt_data.m_sender )
	{
		so_5::rt::mbox_ref_t mbox = evt_data.m_mbox;
		mbox->deliver_message( new msg_hello_to_you( m_agent_name ) );
	}
}

void
a_hello_t::evt_hello_to_you(
	const msg_hello_to_you & evt_data )
{
	std::cout << m_agent_name << ".evt_hello_to_you: "
		<< evt_data.m_sender << std::endl;
}

// The SObjectizer Environment initialization.
void
init( so_5::rt::environment_t & env )
{
	// Creating a cooperation.
	so_5::rt::agent_coop_unique_ptr_t coop = env.create_coop( "coop" );

	// Adding agents to the cooperation.
	coop->add_agent( new a_hello_t( env, "alpha" ) );
	coop->add_agent( new a_hello_t( env, "beta" ) );
	coop->add_agent( new a_hello_t( env, "gamma" ) );

	// Registering the cooperation.
	env.register_coop( std::move( coop ) );

	// Give some time to agents.
	std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	env.stop();
}

int
main( int, char ** )
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
